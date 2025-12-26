# XSigma CI - Windows Dependency Installation Script (PowerShell)
# This script installs all required dependencies for building XSigma on Windows
# Usage: .\install-deps-windows.ps1 [-WithCuda] [-WithTbb]

param(
    [switch]$WithCuda = $false,
    [switch]$WithTbb = $false
)

# Color codes for output
function Write-Info {
    param([string]$Message)
    Write-Host "[INFO] $Message" -ForegroundColor Blue
}

function Write-Success {
    param([string]$Message)
    Write-Host "[SUCCESS] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[WARNING] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[ERROR] $Message" -ForegroundColor Red
}

# Error handling
$ErrorActionPreference = "Stop"

Write-Info "Starting Windows dependency installation..."
Write-Info "CUDA support: $WithCuda"
Write-Info "TBB support: $WithTbb"

# Check if Chocolatey is installed
if (-not (Get-Command choco -ErrorAction SilentlyContinue)) {
    Write-Error "Chocolatey is not installed. Please install Chocolatey first:"
    Write-Info "  Run PowerShell as Administrator and execute:"
    Write-Info "  Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))"
    exit 1
}

Write-Info "Chocolatey found. Installing dependencies..."

# Core build tools
Write-Info "Installing core build tools..."
try {
    choco install -y cmake ninja git curl wget python || {
        Write-Error "Failed to install core build tools"
        exit 1
    }
} catch {
    Write-Error "Failed to install core build tools: $_"
    exit 1
}

# Clang compiler
Write-Info "Installing Clang compiler..."
try {
    choco install -y llvm || {
        Write-Warning "Failed to install LLVM/Clang"
    }
} catch {
    Write-Warning "Failed to install LLVM/Clang: $_"
}

# buildcache compiler cache
Write-Info "Installing buildcache compiler cache..."
$buildcacheVersion = $env:BUILDCACHE_VERSION
if ([string]::IsNullOrWhiteSpace($buildcacheVersion)) {
    $buildcacheVersion = "0.33.0"
}
try {
    $buildcacheRoot = Join-Path $env:USERPROFILE ".local\bin"
    New-Item -ItemType Directory -Force -Path $buildcacheRoot | Out-Null
    $zipPath = Join-Path $env:TEMP "buildcache.zip"
    $buildcacheUrl = "https://github.com/mbitsnbites/buildcache/releases/download/v$buildcacheVersion/buildcache-windows.zip"
    Write-Info "Downloading buildcache from $buildcacheUrl"
    Invoke-WebRequest -Uri $buildcacheUrl -OutFile $zipPath -UseBasicParsing
    Expand-Archive -Path $zipPath -DestinationPath $buildcacheRoot -Force
    Remove-Item $zipPath -Force
    $exe = Get-ChildItem -Path $buildcacheRoot -Filter buildcache.exe -Recurse | Select-Object -First 1
    if (-not $exe) {
        throw "buildcache.exe not found after extraction"
    }
    $targetExe = Join-Path $buildcacheRoot "buildcache.exe"
    Copy-Item $exe.FullName -Destination $targetExe -Force
    if ($exe.DirectoryName -ne $buildcacheRoot) {
        Remove-Item $exe.DirectoryName -Recurse -Force
    }
    if ($env:GITHUB_PATH) {
        Add-Content -Path $env:GITHUB_PATH -Value $buildcacheRoot
    } else {
        $env:Path = "$buildcacheRoot;$env:Path"
    }
    $buildcacheVersionOutput = & $targetExe --version
    Write-Success "buildcache installed: $buildcacheVersionOutput"
} catch {
    Write-Warning "Failed to install buildcache: $_"
}

# Visual Studio Build Tools (for MSVC)
Write-Info "Checking for Visual Studio Build Tools..."
if (-not (Test-Path "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools")) {
    Write-Warning "Visual Studio Build Tools not found. Installing..."
    try {
        choco install -y visualstudio2022buildtools || {
            Write-Warning "Failed to install Visual Studio Build Tools"
        }
    } catch {
        Write-Warning "Failed to install Visual Studio Build Tools: $_"
    }
} else {
    Write-Success "Visual Studio Build Tools found"
}

# TBB (Threading Building Blocks)
if ($WithTbb) {
    Write-Info "Installing Intel TBB..."
    try {
        choco install -y tbb || {
            Write-Warning "Failed to install TBB from Chocolatey"
        }
    } catch {
        Write-Warning "Failed to install TBB: $_"
    }
}

# CUDA Toolkit (optional)
if ($WithCuda) {
    Write-Info "Installing CUDA Toolkit..."
    Write-Warning "CUDA installation skipped - typically pre-installed in CI environment"
}

# Python dependencies
Write-Info "Installing Python dependencies..."
try {
    python -m pip install --upgrade pip setuptools wheel | Out-Null
    python -m pip install colorama==0.4.6 psutil==6.1.1 | Out-Null
    Write-Success "Python dependencies installed"
} catch {
    Write-Warning "Failed to install Python dependencies: $_"
}

# Refresh environment variables
Write-Info "Refreshing environment variables..."
$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User")

# Verify installations
Write-Info "Verifying installations..."

$tools = @("cmake", "ninja", "git", "python", "clang", "buildcache")
foreach ($tool in $tools) {
    if (Get-Command $tool -ErrorAction SilentlyContinue) {
        Write-Success "$tool is available"
    } else {
        Write-Warning "$tool is not available in PATH"
    }
}

Write-Success "Windows dependency installation completed!"
Write-Info "You can now build XSigma using: python Scripts/setup.py ninja clang config build test"
