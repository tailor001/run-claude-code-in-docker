# Python Project Structure and Organization

## Quick Reference

| Component | Recommended Structure | Purpose |
|-----------|---------------------|---------|
| **Source code** | `src/project_name/` | Clean separation, editable installs |
| **Tests** | `tests/` | Separate from source, easy discovery |
| **Configuration** | `pyproject.toml` | Modern Python packaging standard |
| **Documentation** | `docs/` | Project documentation |
| **Scripts** | `scripts/` | Development/maintenance scripts |
| **Docker** | `Dockerfile`, `docker-compose.yml` | Containerization |

## Standard Project Layout

### Basic Project Structure
```
my_project/
├── pyproject.toml           # Project configuration and dependencies
├── README.md               # Project documentation
├── .gitignore             # Git ignore patterns
├── .pre-commit-config.yaml # Pre-commit hooks
├── src/                   # Source code
│   └── my_project/
│       ├── __init__.py    # Package initialization
│       ├── main.py        # Main entry point
│       ├── cli.py         # CLI interface
│       ├── config.py      # Configuration handling
│       └── core/
│           ├── __init__.py
│           ├── models.py  # Data models
│           ├── services.py # Business logic
│           └── utils.py   # Utilities
├── tests/                 # Test suite
│   ├── __init__.py
│   ├── conftest.py       # Pytest configuration
│   ├── unit/
│   │   ├── test_models.py
│   │   └── test_services.py
│   └── integration/
│       └── test_api.py
├── scripts/              # Development scripts
│   ├── setup_dev.py
│   └── migrate_data.py
├── docs/                 # Documentation
│   ├── index.md
│   └── api.md
└── .github/              # GitHub workflows
    └── workflows/
        ├── test.yml
        └── deploy.yml
```

### pyproject.toml Template
```toml
[build-system]
requires = ["hatchling"]
build-backend = "hatchling.build"

[project]
name = "my-project"
version = "0.1.0"
description = "A modern Python project"
readme = "README.md"
requires-python = ">=3.9"
license = {text = "MIT"}
authors = [
    {name = "Your Name", email = "your.email@example.com"},
]
keywords = ["python", "project", "template"]
classifiers = [
    "Development Status :: 4 - Beta",
    "Intended Audience :: Developers",
    "License :: OSI Approved :: MIT License",
    "Programming Language :: Python :: 3",
    "Programming Language :: Python :: 3.9",
    "Programming Language :: Python :: 3.10",
    "Programming Language :: Python :: 3.11",
    "Programming Language :: Python :: 3.12",
]
dependencies = [
    "pydantic>=2.0.0",
    "fastapi>=0.100.0",
]

[project.optional-dependencies]
dev = [
    "pytest>=7.0.0",
    "pytest-cov>=4.0.0",
    "pytest-benchmark>=4.0.0",
    "black>=23.0.0",
    "isort>=5.12.0",
    "ruff>=0.1.0",
    "mypy>=1.0.0",
    "pre-commit>=3.0.0",
]
docs = [
    "mkdocs>=1.5.0",
    "mkdocs-material>=9.0.0",
]

[project.scripts]
my-cli = "my_project.cli:main"

[project.urls]
Homepage = "https://github.com/yourname/my-project"
Documentation = "https://my-project.readthedocs.io/"
Repository = "https://github.com/yourname/my-project.git"
"Bug Tracker" = "https://github.com/yourname/my-project/issues"

[tool.hatch.build.targets.wheel]
packages = ["src/my_project"]
```

## Package Organization Patterns

### 1. Feature-Based Organization
```
src/my_project/
├── __init__.py
├── auth/
│   ├── __init__.py
│   ├── models.py      # User, Token models
│   ├── services.py    # Authentication logic
│   ├── routes.py      # API endpoints
│   └── utils.py       # Password hashing, validation
├── products/
│   ├── __init__.py
│   ├── models.py      # Product models
│   ├── services.py    # Product business logic
│   ├── routes.py      # Product API
│   └── repositories.py # Database access
└── orders/
    ├── __init__.py
    ├── models.py      # Order models
    ├── services.py    # Order processing
    └── workflows.py   # Multi-step processes
```

### 2. Layer-Based Organization
```
src/my_project/
├── __init__.py
├── api/               # API layer
│   ├── __init__.py
│   ├── v1/
│   │   ├── __init__.py
│   │   ├── auth.py
│   │   └── products.py
│   └── dependencies.py
├── core/              # Business logic
│   ├── __init__.py
│   ├── domain.py      # Domain models
│   ├── services.py    # Business services
│   └── events.py      # Domain events
├── infrastructure/    # External concerns
│   ├── __init__.py
│   ├── database.py    # Database setup
│   ├── external_apis.py
│   └── storage.py
└── shared/            # Common utilities
    ├── __init__.py
    ├── exceptions.py
    └── utils.py
```

## __init__.py Patterns

### 1. Minimal __init__.py
```python
"""My Project - A modern Python application."""

__version__ = "0.1.0"
__author__ = "Your Name"
```

### 2. API Control __init__.py
```python
"""Package core module with controlled API."""

from .models import User, Product, Order
from .services import UserService, ProductService, OrderService
from .exceptions import ValidationError, NotFoundError

# Define what's exported when using `from package import *`
__all__ = [
    "User",
    "Product",
    "Order",
    "UserService",
    "ProductService",
    "OrderService",
    "ValidationError",
    "NotFoundError",
]
```

### 3. Namespace Package __init__.py
```python
"""Namespace package for related functionality."""

# Import all submodules to make them available
from . import auth
from . import products
from . import orders

# Convenience imports at package level
from .auth.models import User
from .products.models import Product
from .orders.models import Order

__all__ = ["auth", "products", "orders", "User", "Product", "Order"]
```

### 4. Lazy Loading __init__.py
```python
"""Package with lazy loading for better startup performance."""

def __getattr__(name: str):
    """Lazy import expensive modules."""
    if name == "heavy_service":
        from .services import heavy_service
        globals()["heavy_service"] = heavy_service
        return heavy_service
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
```

## Module Organization

### Single Responsibility Modules
```python
# models.py - Only data models
from pydantic import BaseModel, EmailStr
from datetime import datetime
from typing import Optional

class User(BaseModel):
    """User domain model."""
    id: int
    name: str
    email: EmailStr
    is_active: bool = True
    created_at: datetime
    updated_at: Optional[datetime] = None

class Product(BaseModel):
    """Product domain model."""
    id: int
    name: str
    price: float
    description: str
    created_at: datetime
```

### Service Modules
```python
# services.py - Only business logic
from typing import Optional, List
from .models import User, Product
from .exceptions import NotFoundError, ValidationError

class UserService:
    """Service for user-related operations."""

    def __init__(self, user_repository, email_service):
        self.user_repo = user_repository
        self.email_service = email_service

    async def create_user(self, name: str, email: str) -> User:
        """Create a new user with validation."""
        if await self.user_repo.get_by_email(email):
            raise ValidationError("Email already exists")

        user = User(name=name, email=email)
        created_user = await self.user_repo.save(user)

        await self.email_service.send_welcome_email(email)
        return created_user

    async def get_user(self, user_id: int) -> User:
        """Get user by ID with proper error handling."""
        user = await self.user_repo.get_by_id(user_id)
        if not user:
            raise NotFoundError(f"User {user_id} not found")
        return user
```

### Utility Modules
```python
# utils.py - Pure functions and helpers
import hashlib
import secrets
from typing import Dict, Any

def generate_password_hash(password: str) -> str:
    """Generate secure password hash."""
    salt = secrets.token_hex(16)
    pwd_hash = hashlib.pbkdf2_hmac('sha256',
                                  password.encode('utf-8'),
                                  salt.encode('utf-8'),
                                  100000)
    return f"{salt}${pwd_hash.hex()}"

def validate_email(email: str) -> bool:
    """Simple email validation."""
    return "@" in email and "." in email.split("@")[-1]

def safe_dict_get(data: Dict[str, Any], key: str, default: Any = None) -> Any:
    """Safely get value from dictionary with default."""
    return data.get(key, default)
```

## Dependency Management Patterns

### Modern pyproject.toml Dependencies
```toml
[project.dependencies]
# Core dependencies
fastapi = ">=0.100.0,<0.110.0"
pydantic = ">=2.0.0,<3.0.0"
sqlalchemy = ">=2.0.0,<3.0.0"

# Database drivers
asyncpg = {version = ">=0.28.0", optional = true}
psycopg2-binary = {version = ">=2.9.0", optional = true}

[project.optional-dependencies]
# Development dependencies
dev = [
    "pytest>=7.0.0",
    "pytest-cov>=4.0.0",
    "black>=23.0.0",
    "isort>=5.12.0",
    "ruff>=0.1.0",
    "mypy>=1.0.0",
]

# Database drivers
postgres = ["asyncpg>=0.28.0"]
mysql = ["aiomysql>=0.2.0"]
sqlite = ["aiosqlite>=0.19.0"]

# Production dependencies
production = [
    "gunicorn>=21.0.0",
    "sentry-sdk[fastapi]>=1.0.0",
]

# Extra features
monitoring = ["sentry-sdk>=1.0.0", "prometheus-client>=0.17.0"]
cache = ["redis>=4.5.0", "hiredis>=2.2.0"]
```

### Lock Files and Reproducibility
```bash
# Create lock file for reproducible builds
uv lock

# Install from lock file (exact versions)
uv sync --frozen

# Update specific dependency
uv add requests@latest

# Update all dependencies
uv lock --upgrade

# Export to requirements.txt (if needed for legacy systems)
uv pip compile pyproject.toml -o requirements.txt
```

## Entry Points and Scripts

### CLI Application Entry Point
```python
# src/my_project/cli.py
import click
from .config import Settings
from .services import AppService

@click.group()
@click.option('--config', default='config.toml', help='Configuration file')
@click.pass_context
def cli(ctx, config):
    """My Project CLI Application."""
    ctx.ensure_object(dict)
    ctx.obj['settings'] = Settings.from_file(config)
    ctx.obj['service'] = AppService(ctx.obj['settings'])

@cli.command()
@click.pass_context
def start(ctx):
    """Start the application."""
    service = ctx.obj['service']
    service.start()

@cli.command()
@click.argument('user_id', type=int)
@click.pass_context
def get_user(ctx, user_id):
    """Get user by ID."""
    service = ctx.obj['service']
    user = service.get_user(user_id)
    click.echo(f"User: {user.name} ({user.email})")

if __name__ == '__main__':
    cli()
```

### Package Entry Points
```toml
[project.scripts]
my-app = "my_project.cli:cli"
my-service = "my_project.main:start_service"
worker = "my_project.workers:start_worker"

[project.gui-scripts]
my-gui = "my_project.gui:main"
```

## Configuration Management

### Environment-Specific Configuration
```python
# src/my_project/config.py
from pathlib import Path
from typing import Optional, Dict, Any
from pydantic import BaseSettings, Field

class Settings(BaseSettings):
    """Application settings with environment variable support."""

    # Application settings
    app_name: str = "My Project"
    debug: bool = False
    version: str = "0.1.0"

    # Database settings
    database_url: str = Field(..., env="DATABASE_URL")
    database_pool_size: int = 10

    # Redis settings
    redis_url: Optional[str] = Field(None, env="REDIS_URL")

    # Security settings
    secret_key: str = Field(..., env="SECRET_KEY")
    jwt_algorithm: str = "HS256"

    # External services
    api_key: Optional[str] = Field(None, env="API_KEY")

    class Config:
        env_file = ".env"
        env_file_encoding = "utf-8"
        case_sensitive = False

    @classmethod
    def from_file(cls, config_path: Path) -> "Settings":
        """Load settings from TOML file."""
        import tomllib
        with open(config_path, 'rb') as f:
            config_data = tomllib.load(f)
        return cls(**config_data)

# Environment-specific overrides
class DevelopmentSettings(Settings):
    """Development environment settings."""
    debug: bool = True
    database_url: str = "sqlite:///./dev.db"

class ProductionSettings(Settings):
    """Production environment settings."""
    debug: bool = False
    database_pool_size: int = 20

def get_settings() -> Settings:
    """Get appropriate settings based on environment."""
    env = os.getenv("ENVIRONMENT", "development").lower()

    if env == "production":
        return ProductionSettings()
    elif env == "development":
        return DevelopmentSettings()
    else:
        return Settings()
```

## Common Anti-Patterns

### Directory Structure Anti-Patterns
```python
# Bad - All files in root directory
my_project/
├── main.py
├── models.py
├── views.py
├── utils.py
├── database.py
└── tests.py

# Good - Organized by responsibility
my_project/
├── src/
│   └── my_project/
│       ├── __init__.py
│       ├── models/
│       ├── views/
│       ├── utils/
│       └── database/
└── tests/
    ├── unit/
    └── integration/
```

### Import Anti-Patterns
```python
# Bad - Circular imports
# models.py
from .services import process_data

# services.py
from .models import DataModel

# Good - Dependency injection
# models.py
class DataModel:
    pass

# services.py
from .models import DataModel

class DataProcessor:
    def __init__(self, model_class: type[DataModel]):
        self.model_class = model_class
```

### Configuration Anti-Patterns
```python
# Bad - Hardcoded configuration
API_URL = "https://api.example.com"
DEBUG = True

# Good - Environment-based configuration
import os
from pydantic import BaseSettings

class Settings(BaseSettings):
    api_url: str = Field(..., env="API_URL")
    debug: bool = Field(False, env="DEBUG")
```