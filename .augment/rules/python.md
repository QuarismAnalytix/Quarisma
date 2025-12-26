---
type: "always_apply"
---

# 🐍 Python Coding Rules  
**Google Python Style + Cross-Platform Compliance**

---

## 1. 📘 General Guidelines
- Follow the **[Google Python Style Guide](https://google.github.io/styleguide/pyguide.html)**.
- Target **Python 3.9+** for full typing and compatibility.
- Code must run on **Windows**, **macOS**, and **Linux**.
- Write **modular, readable, and testable** code.

---

## 2. 📁 File and Module Organization
- One module per file (`.py`).
- Filenames use lowercase with underscores: `file_handler.py`.
- Use **`pathlib`** for file paths — avoids OS-specific issues.

```python
from pathlib import Path

data_dir = Path.home() / "Documents" / "project_data"
```

✅ Works on all systems.

---

## 3. 🧱 Imports
Order imports as follows:
1. Standard library
2. Third-party libraries
3. Local modules

Each group separated by a blank line.

```python
import os
import sys

import requests

from myapp import config
```

---

## 4. 📄 Docstrings
Use triple double quotes (`\"\"\"`) for all public functions, classes, and modules.  
Follow **Google-style docstrings**.

```python
def get_user_info(user_id: int) -> dict:
    \"\"\"Fetches user information by ID.

    Args:
        user_id: Unique identifier of the user.

    Returns:
        Dictionary containing user data.

    Raises:
        ValueError: If the ID is invalid.
    \"\"\"
    ...
```

---

## 5. 🧩 Functions and Classes
- Functions and variables: `snake_case`
- Classes: `CapWords`
- Constants: `UPPER_CASE`
- Keep functions short and focused on one purpose.

```python
class FileHandler:
    \"\"\"Handles file operations.\"\"\"

    def read_text(self, filepath: str) -> str:
        \"\"\"Reads text from a file.\"\"\"
        with open(filepath, encoding="utf-8") as file:
            return file.read()
```

---

## 6. 🌐 Cross-Platform Practices
✅ **DO:**
- Use `pathlib`, `os`, `subprocess`, `tempfile`
- Use environment variables (`os.environ`)
- Handle newline differences with `newline=""` in file I/O

🚫 **DON’T:**
- Use hard-coded file paths (`C:\\`, `/home/`)
- Use OS-specific commands (`cls`, `clear`)

```python
import subprocess

subprocess.run(["echo", "Hello World!"], check=True)
```

---

## 7. ⚙️ Error Handling
Catch **specific exceptions** and include meaningful error messages.

```python
try:
    process_data()
except FileNotFoundError as e:
    print(f"File missing: {e}")
```

---

## 8. 🧪 Testing
- Use `unittest` or `pytest`
- Test filenames start with `test_`
- Keep tests independent of the environment

```python
import unittest

class TestMath(unittest.TestCase):
    def test_addition(self):
        self.assertEqual(2 + 2, 4)
```

Run tests:
```bash
pytest tests/
```

---

## 9. 🧼 Code Quality
Use automatic tools to enforce quality and formatting.

### Format with `black`
```bash
black --line-length 80 src/
```

### Lint with `pylint` or `ruff`
```bash
pylint src/
```

---

## 10. 🔒 Security and Portability
- No hard-coded credentials or absolute paths.
- Use environment variables or `.env` files.
- Use only cross-platform standard library modules unless absolutely needed.

---

## ✅ Summary Checklist

| Rule | Description | Example |
|------|--------------|----------|
| Style | Google Python Style | `snake_case`, `\"\"\"Docstrings\"\"\"` |
| Imports | Group by category | stdlib → 3rd → local |
| Paths | Use `pathlib` | ✅ |
| Exceptions | Catch specific | `except ValueError:` |
| Tests | Use `pytest` or `unittest` | `test_*.py` |
| Lint | `pylint`, `ruff` | Static checks |
| Format | `black` | Enforces style |
| Security | Use env vars | No hard-coded secrets |

---

# ⚙️ Configuration Files

## 🧩 `pyproject.toml` (for **black** and **ruff**)

```toml
[tool.black]
line-length = 80
target-version = ['py39']
skip-string-normalization = false

[tool.ruff]
line-length = 80
target-version = "py39"
select = ["E", "F", "W", "C", "I"]
ignore = ["E203", "E501", "W503"]
fix = true
exclude = ["venv", "__pycache__"]

[tool.ruff.isort]
known-third-party = ["requests", "pytest"]
combine-as-imports = true
```

---

## 🧹 `.pylintrc` (Google-style compatible)

```ini
[MASTER]
ignore=tests
jobs=1
extension-pkg-whitelist=
persistent=yes
suggestion-mode=yes

[MESSAGES CONTROL]
disable=
    C0114,  ; missing-module-docstring
    C0115,  ; missing-class-docstring
    C0116,  ; missing-function-docstring
    R0903,  ; too-few-public-methods

[FORMAT]
max-line-length=80
indent-string='    '

[DESIGN]
max-args=5
max-locals=15
max-returns=6
max-branches=12
max-statements=50

[REPORTS]
output-format=colorized
score=yes

[TYPECHECK]
ignored-modules=requests
```

---

## 📦 Recommended Folder Layout
```
project_root/
├── src/
│   ├── __init__.py
│   ├── main.py
│   ├── utils.py
│   └── handlers/
├── tests/
│   ├── __init__.py
│   └── test_utils.py
├── pyproject.toml
├── .pylintrc
└── README.md
```

---

## 🚀 Quick Setup

```bash
# Install tools
pip install black ruff pylint pytest

# Check formatting & linting
black --check src/
ruff check src/
pylint src/

# Run tests
pytest
```

---

**Author:** Your Development Team  
**Standard:** Google Python Style + Cross-Platform Ready  
**Version:** 1.0.0  
**License:** MIT
