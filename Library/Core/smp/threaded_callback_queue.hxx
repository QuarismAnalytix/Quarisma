

// This file incorporates code from the Visualization Toolkit (VTK) and remains subject to the BSD-3-Clause VTK license.

#include <array>
#include <cassert>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace xsigma
{

//-----------------------------------------------------------------------------
template <>
struct threaded_callback_queue::ReturnValueWrapper<void, false>
{
    using ReturnLValueRef      = void;
    using ReturnConstLValueRef = void;

    void Get() const {}
};

//=============================================================================
template <class ReturnT>
struct threaded_callback_queue::ReturnValueWrapper<ReturnT, true /* IsLValueReference */>
{
    using ReturnValueImpl      = ReturnValueWrapper<ReturnT, false>;
    using ReturnLValueRef      = ReturnT&;
    using ReturnConstLValueRef = const ReturnT&;

    ReturnValueWrapper() = default;
    ReturnValueWrapper(ReturnT& value)
        : Value(std::unique_ptr<ReturnValueImpl>(new ReturnValueImpl(value)))
    {
    }

    ReturnT& Get() { return this->Value->Get(); }

    const ReturnT& Get() const { return this->Value->Get(); }

    std::unique_ptr<ReturnValueImpl> Value;
};

//=============================================================================
template <class ReturnT>
struct threaded_callback_queue::ReturnValueWrapper<ReturnT, false /* IsLValueReference */>
{
    using ReturnLValueRef      = ReturnT&;
    using ReturnConstLValueRef = const ReturnT&;

    ReturnValueWrapper() = default;
    template <class ReturnTT>
    ReturnValueWrapper(ReturnTT&& value)  // NOLINT(bugprone-forwarding-reference-overload)
        : Value(std::forward<ReturnTT>(value))
    {
    }

    ReturnT& Get() { return this->Value; }

    const ReturnT& Get() const { return this->Value; }

    ReturnT Value;
};

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::xsigmaSharedFuture<ReturnT>::ReturnLValueRef
threaded_callback_queue::xsigmaSharedFuture<ReturnT>::Get()
{
    this->Wait();
    return this->ReturnValue.Get();
}

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::xsigmaSharedFuture<ReturnT>::ReturnConstLValueRef
threaded_callback_queue::xsigmaSharedFuture<ReturnT>::Get() const
{
    this->Wait();
    return this->ReturnValue.Get();
}

//=============================================================================
struct threaded_callback_queue::InvokerImpl
{
    /**
   * Substitute for std::integer_sequence which is C++14
   */
    template <std::size_t... Is>
    struct IntegerSequence;
    template <std::size_t N, std::size_t... Is>
    struct MakeIntegerSequence;

    /**
   * This function is used to discriminate whether you can dereference the template parameter T.
   * It uses SNFINAE and jumps to the std::false_type version if it fails dereferencing T.
   */
    template <class T>
    static decltype(*std::declval<T&>(), std::true_type{}) CanBeDereferenced(std::nullptr_t);
    template <class>
    static std::false_type CanBeDereferenced(...);

    template <class T, class CanBeDereferencedT = decltype(CanBeDereferenced<T>(nullptr))>
    struct DereferenceImpl;

    /**
   * Convenient typedef that use Signature to convert the parameters of function of type FT
   * to a std::tuple.
   */
    template <class FT, std::size_t N = Signature<typename std::decay<FT>::type>::ArgsSize>
    using ArgsTuple = typename Signature<typename std::decay<FT>::type>::ArgsTuple;

    /**
   * Convenient typedef that, given a function of type FT and an index I, returns the type of the
   * Ith parameter of the function.
   */
    template <class FT, std::size_t I>
    using ArgType = typename std::tuple_element<I, ArgsTuple<FT>>::type;

    /**
   * This helper function returns a tuple of cherry-picked types from the argument types from
   * the input function or the argument types of the provided input following this criterion:
   * * If the input function expects an lvalue reference, store it in its decayed type.
   * * Otherwise, store it as is (decayed input type). The conversion to the function argument
   *   type will be done upon invoking.
   *
   * This specific casting allows us to permit calling the constructor for rvalue reference inputs
   * when the type differs from the one provided by the user (take a const char* input when the
   * function expects a std::string for instance).
   * We want to keep the input type in all other circumstances in case the user inputs smart
   * pointers and the function only expects a raw pointer. If we casted it to the function argument
   * type, we would not own a reference of the smart pointer.
   *
   * FunctionArgsTupleT is a tuple of the function native argument types (extracted from its
   * signature), and InputArgsTupleT is a tuple of the types provided by the user as input
   * parameters.
   */
    template <class FunctionArgsTupleT, class InputArgsTupleT, std::size_t... Is>
    static std::tuple<typename std::conditional<
        std::is_lvalue_reference<typename std::tuple_element<Is, FunctionArgsTupleT>::type>::value,
        typename std::decay<typename std::tuple_element<Is, FunctionArgsTupleT>::type>::type,
        typename std::decay<typename std::tuple_element<Is, InputArgsTupleT>::type>::type>::type...>
        GetStaticCastArgsTuple(IntegerSequence<Is...>);

    /**
   * Convenient typedef to create a tuple of types allowing to call the constructor of the function
   * argument type when relevant, or hold a copy of the input parameters provided by the user
   * instead.
   */
    template <class FunctionArgsTupleT, class... InputArgsT>
    using StaticCastArgsTuple =
        decltype(GetStaticCastArgsTuple<FunctionArgsTupleT, std::tuple<InputArgsT...>>(
            MakeIntegerSequence<sizeof...(InputArgsT)>()));

    /**
   * This holds the attributes of a function.
   * There are 2 implementations: one for member function pointers, and one for all the others
   * (functors, lambdas, function pointers)
   */
    template <bool IsMemberFunctionPointer, class... ArgsT>
    class InvokerHandle;

    /**
   * Actually invokes the function and sets its future. An specific implementation is needed for
   * void return types.
   */
    template <class ReturnT>
    struct InvokerHelper
    {
        template <class InvokerT>
        static void Invoke(InvokerT&& invoker, xsigmaSharedFuture<ReturnT>* future)
        {
            future->ReturnValue = ReturnValueWrapper<ReturnT>(invoker());
            future->Status.store(READY, std::memory_order_release);
            future->ConditionVariable.notify_all();
        }
    };
};

//=============================================================================
template <>
struct threaded_callback_queue::InvokerImpl::InvokerHelper<void>
{
    template <class InvokerT>
    static void Invoke(InvokerT&& invoker, xsigmaSharedFuture<void>* future)
    {
        invoker();
        future->Status.store(READY, std::memory_order_release);
        future->ConditionVariable.notify_all();
    }
};

//=============================================================================
// For lambdas or std::function
template <class ReturnT, class... ArgsT>
struct threaded_callback_queue::Signature<ReturnT(ArgsT...)>
{
    using ArgsTuple                       = std::tuple<ArgsT...>;
    using InvokeResult                    = ReturnT;
    static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct threaded_callback_queue::Signature<ReturnT (ClassT::*)(ArgsT...)>
{
    using ArgsTuple                       = std::tuple<ArgsT...>;
    using InvokeResult                    = ReturnT;
    static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For const methods inside a class ClassT
template <class ClassT, class ReturnT, class... ArgsT>
struct threaded_callback_queue::Signature<ReturnT (ClassT::*)(ArgsT...) const>
{
    using ArgsTuple                       = std::tuple<ArgsT...>;
    using InvokeResult                    = ReturnT;
    static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For function pointers
template <class ReturnT, class... ArgsT>
struct threaded_callback_queue::Signature<ReturnT (*)(ArgsT...)>
{
    using ArgsTuple                       = std::tuple<ArgsT...>;
    using InvokeResult                    = ReturnT;
    static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For function pointers
template <class ReturnT, class... ArgsT>
struct threaded_callback_queue::Signature<ReturnT (&)(ArgsT...)>
{
    using ArgsTuple                       = std::tuple<ArgsT...>;
    using InvokeResult                    = ReturnT;
    static constexpr std::size_t ArgsSize = sizeof...(ArgsT);
};

//=============================================================================
// For functors
template <class FT>
struct threaded_callback_queue::Signature
    : threaded_callback_queue::Signature<decltype(&FT::operator())>
{
};

//=============================================================================
template <std::size_t... Is>
struct threaded_callback_queue::InvokerImpl::IntegerSequence
{
};

//=============================================================================
template <std::size_t N, std::size_t... Is>
struct threaded_callback_queue::InvokerImpl::MakeIntegerSequence
    : threaded_callback_queue::InvokerImpl::MakeIntegerSequence<N - 1, N - 1, Is...>
{
};

//=============================================================================
template <std::size_t... Is>
struct threaded_callback_queue::InvokerImpl::MakeIntegerSequence<0, Is...>
    : threaded_callback_queue::InvokerImpl::IntegerSequence<Is...>
{
};

//=============================================================================
template <class T>
struct threaded_callback_queue::InvokerImpl::
    DereferenceImpl<T, std::true_type /* CanBeDereferencedT */>
{
    using Type = decltype(*std::declval<T>());
    static Type& Get(T& instance) { return *instance; }
};

//=============================================================================
template <class T>
struct threaded_callback_queue::InvokerImpl::
    DereferenceImpl<T, std::false_type /* CanBeDereferencedT */>
{
    using Type = T;
    static Type& Get(T& instance) { return instance; }
};

//=============================================================================
template <class T>
struct threaded_callback_queue::Dereference<T, std::nullptr_t>
{
    using Type = typename InvokerImpl::DereferenceImpl<T>::Type;
};

//=============================================================================
template <class FT, class ObjectT, class... ArgsT>
class threaded_callback_queue::InvokerImpl::
    InvokerHandle<true /* IsMemberFunctionPointer */, FT, ObjectT, ArgsT...>
{
public:
    template <class FTT, class ObjectTT, class... ArgsTT>
    InvokerHandle(FTT&& f, ObjectTT&& instance, ArgsTT&&... args)
        : Function(std::forward<FTT>(f)),
          Instance(std::forward<ObjectTT>(instance)),
          Args(std::forward<ArgsTT>(args)...)
    {
    }

    InvokeResult<FT> operator()() { return this->Invoke(MakeIntegerSequence<sizeof...(ArgsT)>()); }

private:
    template <std::size_t... Is>
    InvokeResult<FT> Invoke(IntegerSequence<Is...>)
    {
        // If the input object is wrapped inside a pointer (could be shared_ptr, xsigmaSmartPointer),
        // we need to dereference the object before invoking it.
        auto& deref = DereferenceImpl<ObjectT>::Get(this->Instance);

        // The static_cast to ArgType forces casts to the correct types of the function.
        // There are conflicts with rvalue references not being able to be converted to lvalue
        // references if this static_cast is not performed
        return (deref.*Function)(static_cast<ArgType<FT, Is>>(std::get<Is>(this->Args))...);
    }

    FT Function;

    // We DO NOT want to hold lvalue references! They could be destroyed before we execute them.
    // This forces to call the copy constructor on lvalue references inputs.
    typename std::decay<ObjectT>::type Instance;

    // We want to hold an instance of the arguments in the type expected by the function rather than
    // the types provided by the user when the function expects a lvalue reference.
    // This way, if there is a conversion to be done, it can be done
    // in the constructor of each type.
    //
    // Example: The user provides a string as "example", but the function expects a std::string&.
    // We can directly store this argument as a std::string and allow to pass it to the function as a
    // std::string.
    StaticCastArgsTuple<ArgsTuple<FT>, ArgsT...> Args;
};

//=============================================================================
template <class FT, class... ArgsT>
class threaded_callback_queue::InvokerImpl::
    InvokerHandle<false /* IsMemberFunctionPointer */, FT, ArgsT...>
{
public:
    template <class FTT, class... ArgsTT>
    InvokerHandle(FTT&& f, ArgsTT&&... args)
        : Function(std::forward<FTT>(f)), Args(std::forward<ArgsTT>(args)...)
    {
    }

    InvokeResult<FT> operator()() { return this->Invoke(MakeIntegerSequence<sizeof...(ArgsT)>()); }

private:
    template <std::size_t... Is>
    InvokeResult<FT> Invoke(IntegerSequence<Is...>)
    {
        // If the input is a functor and is wrapped inside a pointer (could be shared_ptr),
        // we need to dereference the functor before invoking it.
        auto& f = DereferenceImpl<FT>::Get(this->Function);

        // The static_cast to ArgType forces casts to the correct types of the function.
        // There are conflicts with rvalue references not being able to be converted to lvalue
        // references if this static_cast is not performed
        return f(static_cast<ArgType<decltype(f), Is>>(std::get<Is>(this->Args))...);
    }

    // We DO NOT want to hold lvalue references! They could be destroyed before we execute them.
    // This forces to call the copy constructor on lvalue references inputs.
    typename std::decay<FT>::type Function;

    // We want to hold an instance of the arguments in the type expected by the function rather than
    // the types provided by the user when the function expects a lvalue reference.
    // This way, if there is a conversion to be done, it can be done
    // in the constructor of each type.
    //
    // Example: The user provides a string as "example", but the function expects a std::string&.
    // We can directly store this argument as a std::string and allow to pass it to the function as a
    // std::string.
    StaticCastArgsTuple<ArgsTuple<typename Dereference<FT>::Type>, ArgsT...> Args;
};

//=============================================================================
template <class FT, class... ArgsT>
class threaded_callback_queue::xsigmaInvoker
    : public threaded_callback_queue::xsigmaSharedFuture<threaded_callback_queue::InvokeResult<FT>>
{
public:
    template <class... ArgsTT>
    static xsigmaInvoker<FT, ArgsT...>* New(ArgsTT&&... args)
    {
        auto result = new xsigmaInvoker<FT, ArgsT...>(std::forward<ArgsTT>(args)...);
        result->InitializeObjectBase();
        return result;
    }

    template <class... ArgsTT>
    xsigmaInvoker(ArgsTT&&... args) : Impl(std::forward<ArgsTT>(args)...)
    {
    }

    void operator()() override
    {
        assert(
            this->Status.load(std::memory_order_relaxed) == RUNNING && "Status should be RUNNING");
        InvokerImpl::InvokerHelper<InvokeResult<FT>>::Invoke(this->Impl, this);
    }

    friend class threaded_callback_queue;

private:
    InvokerImpl::InvokerHandle<std::is_member_function_pointer<FT>::value, FT, ArgsT...> Impl;

    xsigmaInvoker(const xsigmaInvoker<FT, ArgsT...>& other)  = delete;
    void operator=(const xsigmaInvoker<FT, ArgsT...>& other) = delete;
};

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT, class InvokerT>
void threaded_callback_queue::HandleDependentInvoker(
    SharedFutureContainerT&& priorSharedFutures, InvokerT&& invoker)
{
    // We look at all the dependent futures. Each time we find one, we notify the corresponding
    // invoker that we are waiting.
    // When the signaled invokers terminate, the counter will be decreased, and when it reaches
    // zero, this invoker will be ready to run.
    if (!priorSharedFutures.empty())
    {
        for (const auto& prior : priorSharedFutures)
        {
            // We can do a quick check to avoid locking if possible. If the prior shared future is ready,
            // we can just move on.
            if (prior->Status.load(std::memory_order_acquire) == READY)
            {
                continue;
            }

            // We need to lock the shared state (so we block the invoker side).
            // This way, we can make sure that if the invoker is still running, we notify it that we
            // depend on it before it checks its dependents in SignalDependentSharedFutures
            std::unique_lock<std::mutex> lock(prior->Mutex);
            if (prior->Status.load(std::memory_order_acquire) != READY)
            {
                // We notify the invoker we depend on by adding ourselves in DependentSharedFutures.
                prior->Dependents.emplace_back(invoker);

                // This does not need to be locked because the shared state of this future is not done
                // constructing yet, so the invoker in SignalDependentSharedFutures will never try do
                // anything with it. And it is okay, because at the end of the day, we increment, the
                // invoker decrements, and if we end up with 0 remaining prior futures, we execute the
                // invoker anyway, so the invoker side has nothing to do.
                lock.unlock();
                invoker->NumberOfPriorSharedFuturesRemaining.fetch_add(
                    1, std::memory_order_release);
            }
        }
    }
    // We notify every invokers we depend on that we are done constructing.
    std::unique_lock<std::mutex> lock(invoker->Mutex);
    if (invoker->NumberOfPriorSharedFuturesRemaining)
    {
        invoker->Status.store(ON_HOLD, std::memory_order_release);
    }
    else
    {
        invoker->Status.store(RUNNING, std::memory_order_release);
        lock.unlock();
        this->Invoke(std::forward<InvokerT>(invoker));
    }
}

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT>
bool threaded_callback_queue::MustWait(SharedFutureContainerT&& priorSharedFutures)
{
    for (const auto& prior : priorSharedFutures)
    {
        if (prior->Status.load(std::memory_order_acquire) != READY)
        {
            return true;
        }
    }
    return false;
}

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT>
void threaded_callback_queue::Wait(SharedFutureContainerT&& priorSharedFutures)
{
    int mustWait = false;

    // First pass: we look if we find any prior that is neither on hold, constructing,
    // ready or running.
    // This means that the associated invoker is enqueued and waiting. We can take care of it
    // and save time instead of waiting.
    for (ptr_mutable<xsigmaSharedFutureBase> prior : priorSharedFutures)
    {
        switch (prior->Status.load(std::memory_order_acquire))
        {
        case RUNNING:
        case ON_HOLD:
        case CONSTRUCTING:
            mustWait = true;
            break;
        case ENQUEUED:
            mustWait |= !this->TryInvoke(prior);
            break;
        }
    }

    if (!mustWait || !this->MustWait(std::forward<SharedFutureContainerT>(priorSharedFutures)))
    {
        return;
    }

    // Second pass:
    // Some priors are not ready...
    // We create an invoker and a future with an empty lambda.
    // The idea is to pass the prior shared futures to the routine HandleDependentInvoker.
    // If any prior is not done, the created invoker will be placed in InvokersOnHold and launched
    // automatically when it is ready.
    // We can just wait on the shared future we just created
    auto emptyLambda = [] {};
    auto invoker =
        util::make_ptr_mutable<InvokerPointer<decltype(emptyLambda)>>(std::move(emptyLambda));

    // We notify whoever harvests this invoker that we want to be run right away and not pushed in the
    // InvokerQueue.
    invoker->IsHighPriority = true;

    this->HandleDependentInvoker(std::forward<SharedFutureContainerT>(priorSharedFutures), invoker);

    invoker->Wait();
}

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::xsigmaSharedFuture<ReturnT>::ReturnLValueRef
threaded_callback_queue::Get(SharedFuturePointer<ReturnT>& future)
{
    this->Wait(std::array<xsigmaSharedFuture<ReturnT>*, 1>{future});
    return future->Get();
}

//-----------------------------------------------------------------------------
template <class ReturnT>
typename threaded_callback_queue::xsigmaSharedFuture<ReturnT>::ReturnConstLValueRef
threaded_callback_queue::Get(const SharedFuturePointer<ReturnT>& future)
{
    this->Wait(std::array<xsigmaSharedFuture<ReturnT>*, 1>{future});
    return future->Get();
}

//-----------------------------------------------------------------------------
template <class SharedFutureContainerT, class FT, class... ArgsT>
threaded_callback_queue::SharedFuturePointer<threaded_callback_queue::InvokeResult<FT>>
threaded_callback_queue::PushDependent(
    SharedFutureContainerT&& priorSharedFutures, FT&& f, ArgsT&&... args)
{
    // If we can avoid doing tricks with dependent shared futures, let's do it.
    if (!this->MustWait(std::forward<SharedFutureContainerT>(priorSharedFutures)))
    {
        return this->Push(std::forward<FT>(f), std::forward<ArgsT>(args)...);
    }

    using InvokerPointerType = InvokerPointer<FT, ArgsT...>;

    auto invoker = util::make_ptr_mutable<InvokerPointerType>(
        std::forward<FT>(f), std::forward<ArgsT>(args)...);

    this->Push(
        &threaded_callback_queue::
            HandleDependentInvoker<SharedFutureContainerT, InvokerPointerType>,
        this,
        std::forward<SharedFutureContainerT>(priorSharedFutures),
        invoker);

    return invoker;
}

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
void threaded_callback_queue::PushControl(FT&& f, ArgsT&&... args)
{
    struct Worker
    {
        void operator()(threaded_callback_queue* self, FT&& _f, ArgsT&&... _args)
        {
            _f(std::forward<ArgsT>(_args)...);
            std::lock_guard<std::mutex> lock(self->ControlMutex);
            self->ControlFutures.erase(this->Future);
        }

        SharedFutureBasePointer Future;
    };

    Worker worker;

    using InvokerPointerType = InvokerPointer<Worker, threaded_callback_queue*, FT, ArgsT...>;

    auto invoker = util::make_ptr_mutable<InvokerPointerType>(
        worker, this, std::forward<FT>(f), std::forward<ArgsT>(args)...);
    worker.Future = invoker;

    // We want the setting of ControlFutures to be strictly sequential. We don't want race conditions
    // on this container with 2 `PushControl` that are called almost simultaneously and have invokers
    // depend on flaky futures.
    auto localControlFutures = [this, &invoker]
    {
        std::lock_guard<std::mutex> lock(this->ControlMutex);

        // We create a copy of the control futures that doesn't have ourselves in yet.
        auto result = this->ControlFutures;
        this->ControlFutures.emplace(invoker);
        return result;
    }();

    // If we can avoid doing tricks with dependent shared futures, let's do it.
    if (!this->MustWait(localControlFutures))
    {
        // The queue is not running yet, we need to invoke by hand.
        if (this->Threads.empty())
        {
            // No need to synchronize anything here. We are the only invoker that is allowed to run here.
            invoker->Status.store(RUNNING, std::memory_order_relaxed);
            (*invoker)();
            return;
        }

        {
            std::lock_guard<std::mutex> invokerLock(invoker->Mutex);
            invoker->Status.store(ENQUEUED, std::memory_order_release);

            std::lock_guard<std::mutex> lock(this->Mutex);
            invoker->InvokerIndex =
                this->InvokerQueue.empty() ? 0 : this->InvokerQueue.front()->InvokerIndex - 1;

            // We give some priority to controls, we push them in the front.
            this->InvokerQueue.emplace_front(invoker);
        }
        this->ConditionVariable.notify_one();
        return;
    }

    // Controls must be run ASAP
    invoker->IsHighPriority = true;

    // Invoker will probably end up on hold and be ran automatically when prior controls have
    // terminated.
    this->HandleDependentInvoker(localControlFutures, invoker);
}

//-----------------------------------------------------------------------------
template <class FT, class... ArgsT>
threaded_callback_queue::SharedFuturePointer<threaded_callback_queue::InvokeResult<FT>>
threaded_callback_queue::Push(FT&& f, ArgsT&&... args)
{
    auto invoker = util::make_ptr_mutable<InvokerPointer<FT, ArgsT...>>(
        std::forward<FT>(f), std::forward<ArgsT>(args)...);
    invoker->Status.store(ENQUEUED, std::memory_order_release);

    {
        std::lock_guard<std::mutex> lock(this->Mutex);
        invoker->InvokerIndex =
            this->InvokerQueue.empty() ? 0 : this->InvokerQueue.back()->InvokerIndex + 1;
        this->InvokerQueue.emplace_back(invoker);
    }

    this->ConditionVariable.notify_one();

    return invoker;
}

}  // namespace xsigma
