# Python Code Style and Formatting

## Quick Reference

| Practice | Tool | Command | Config Location |
|----------|------|---------|-----------------|
| **Code formatting** | black | `uv run black .` | `pyproject.toml` |
| **Import sorting** | isort | `uv run isort .` | `pyproject.toml` |
| **Linting** | ruff | `uv run ruff check .` | `pyproject.toml` |
| **Type checking** | mypy | `uv run mypy src/` | `pyproject.toml` |
| **Security scanning** | bandit | `uv run bandit -r src/` | `pyproject.toml` |

## Configuration Template

### pyproject.toml
```toml
[tool.black]
line-length = 88
target-version = ['py311']
include = '\.pyi?$'
extend-exclude = '''
/(
  # directories
  \.eggs
  | \.git
  | \.hg
  | \.mypy_cache
  | \.tox
  | \.venv
  | build
  | dist
)/
'''

[tool.isort]
profile = "black"
multi_line_output = 3
line_length = 88
known_first_party = ["src"]
known_third_party = ["pytest", "fastapi", "pydantic"]

[tool.ruff]
line-length = 88
select = [
    "E",  # pycodestyle errors
    "W",  # pycodestyle warnings
    "F",  # pyflakes
    "I",  # isort
    "B",  # flake8-bugbear
    "C4", # flake8-comprehensions
    "UP", # pyupgrade
    "ARG001", # unused arguments in functions
]
ignore = [
    "E501",  # line too long, handled by black
    "B008",  # do not perform function calls in argument defaults
]
unfixable = [
    "B",  # avoid fixing bugbear
]

[tool.ruff.per-file-ignores]
"tests/*" = ["ARG001", "S101"]  # Allow unused args and assert in tests

[tool.mypy]
python_version = "3.11"
warn_return_any = true
warn_unused_configs = true
disallow_untyped_defs = true
disallow_incomplete_defs = true
check_untyped_defs = true
disallow_untyped_decorators = true
no_implicit_optional = true
warn_redundant_casts = true
warn_unused_ignores = true
warn_no_return = true
warn_unreachable = true
strict_equality = true

[[tool.mypy.overrides]]
module = [
    "tests.*",
]
disallow_untyped_defs = false

[tool.bandit]
exclude_dirs = ["tests", "test_*"]
skips = ["B101", "B601"]  # Skip assert_used and shell_injection
```

## Type Hint Patterns

### Basic Type Hints
```python
# Good - precise type hints
from typing import List, Dict, Optional, Union, Callable, TypeVar
from collections.abc import Sequence, Mapping
from dataclasses import dataclass
from enum import Enum

# Function signatures
def process_items(
    items: Sequence[str],
    config: Dict[str, Union[str, int]],
    callback: Optional[Callable[[str], bool]] = None
) -> List[str]:
    """Process items with optional callback."""
    return [item for item in items if callback is None or callback(item)]

# Class definitions
class DataManager:
    def __init__(self, initial_data: Mapping[str, int]) -> None:
        self._data: Dict[str, int] = dict(initial_data)

    def get_value(self, key: str, default: int = 0) -> int:
        return self._data.get(key, default)
```

### Advanced Type Patterns
```python
# Generics
T = TypeVar('T')

class Repository(Generic[T]):
    def __init__(self, items: Sequence[T]) -> None:
        self._items = list(items)

    def find(self, predicate: Callable[[T], bool]) -> Optional[T]:
        return next((item for item in self._items if predicate(item)), None)

# Protocol for duck typing
from typing import Protocol, runtime_checkable

@runtime_checkable
class Writable(Protocol):
    def write(self, data: bytes) -> int: ...

def write_to_stream(stream: Writable, data: bytes) -> int:
    return stream.write(data)

# NewType for semantic clarity
from typing import NewType

UserId = NewType('UserId', int)
ProductId = NewType('ProductId', str)

def get_user(user_id: UserId) -> Optional[User]:
    return user_service.get_user(int(user_id))

# Union types
JsonValue = Union[str, int, float, bool, None, Dict[str, Any], List[Any]]
```

## Docstring Conventions

### Google Style (Preferred)
```python
def calculate_compound_interest(
    principal: float,
    rate: float,
    time: int,
    compounds_per_year: int = 1
) -> float:
    """Calculate compound interest using the standard formula.

    This function calculates the final amount after applying compound interest
    over a specified time period.

    Args:
        principal: The initial amount of money invested.
        rate: Annual interest rate as a decimal (e.g., 0.05 for 5%).
        time: Number of years the money is invested.
        compounds_per_year: Number of times interest compounds per year.
            Defaults to 1 (annually).

    Returns:
        The final amount after compound interest.

    Raises:
        ValueError: If principal is negative or rate is less than -1.

    Example:
        >>> calculate_compound_interest(1000, 0.05, 5, 12)
        1283.35
    """
    if principal < 0:
        raise ValueError("Principal cannot be negative")
    if rate < -1:
        raise ValueError("Rate cannot be less than -100%")

    return principal * (1 + rate / compounds_per_year) ** (compounds_per_year * time)
```

### Class Docstrings
```python
@dataclass
class UserProfile:
    """User profile with authentication and personal information.

    Attributes:
        user_id: Unique identifier for the user.
        email: User's email address (must be valid email format).
        full_name: User's complete name.
        is_active: Whether the user account is currently active.
        created_at: Timestamp when the user account was created.
    """

    user_id: UserId
    email: str
    full_name: str
    is_active: bool = True
    created_at: datetime = field(default_factory=datetime.utcnow)

    def __post_init__(self) -> None:
        if not self._is_valid_email(self.email):
            raise ValueError(f"Invalid email address: {self.email}")

    @staticmethod
    def _is_valid_email(email: str) -> bool:
        """Simple email validation."""
        return "@" in email and "." in email.split("@")[-1]
```

## Import Organization Patterns

### Import Order (handled by isort)
```python
# Standard library imports
import os
import sys
from pathlib import Path
from typing import List, Optional

# Third-party imports
import numpy as np
import pandas as pd
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

# Local imports
from .config import Settings
from .models import User
from .utils import format_date
```

### Avoid Wildcard Imports
```python
# Bad
from module import *

# Good - explicit imports
from module import function1, Class2, CONSTANT3

# Even better for large modules
import module
module.function1()
```

## Code Organization

### Module Structure
```python
"""
Module level docstring explaining the module's purpose.
"""

# Constants first
DEFAULT_TIMEOUT = 30
MAX_RETRIES = 3

# Imports
import logging
from typing import Optional

# Module-level logger
logger = logging.getLogger(__name__)

# Class definitions
class DataProcessor:
    """Main data processing class."""
    pass

# Function definitions
def utility_function(data: str) -> str:
    """Utility function at module level."""
    return data.strip()

# Main execution guard
if __name__ == "__main__":
    # Code for when module is run directly
    pass
```

## Anti-Patterns to Avoid

### Common Mistakes
```python
# Bad - inconsistent use of type hints
def mixed_types(data):
    # Some functions have hints, others don't
    return data.strip()

def good_types(data: str) -> str:
    # Be consistent with type hints
    return data.strip()

# Bad - vague type hints
def process_data(data: Any) -> Any:
    return data

# Good - specific type hints
def process_numbers(numbers: List[int]) -> int:
    return sum(numbers)

# Bad - ignoring mypy errors
def func_returns_str() -> int:  # mypy will flag this
    return "hello"  # This will cause runtime issues

# Bad - unnecessary type comments (use type hints instead)
def old_style(x):  # type: (int) -> str
    return str(x)

# Good - modern type hints
def modern_style(x: int) -> str:
    return str(x)
```