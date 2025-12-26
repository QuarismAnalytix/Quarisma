---
type: "always_apply"
description: "build"
---

rule: build_process_rule
description: >
  Ensures the correct build sequence:
  1. Change directory to 'Scripts'.
  2. Run 'python setup.py config.build.test.ninja.clang.python'.
  3. If 'build_ninja_python' directory does not exist,
     run 'python setup.py config.build.test.ninja.clang.python' first.

steps:
  - step:
      action: cd
      target: Scripts
      comment: "Navigate to the Scripts directory before building."

  - step:
      condition:
        directory_not_exists: ../build_ninja_python
      action: run
      command: python setup.py config.build.test.ninja.clang.python
      comment: "Run config step if build_ninja_python folder does not exist."

  - step:
      action: run
      command: python setup.py config.build.test.ninja.clang.python
      comment: "Run the build test step."
