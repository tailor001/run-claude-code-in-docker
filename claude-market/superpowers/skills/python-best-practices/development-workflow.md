# Modern Python Development Workflow

## Quick Reference

| Task | Command | Description |
|------|---------|-------------|
| **Initialize project** | `uv init project-name` | Creates new project with pyproject.toml |
| **Create environment** | `uv venv` | Creates virtual environment in .venv |
| **Add dependency** | `uv add package-name` | Adds to dependencies and installs |
| **Add dev dependency** | `uv add --dev pytest` | Adds to dev dependencies only |
| **Run commands** | `uv run python script.py` | Executes in project environment |
| **Run tests** | `uv run pytest` | Runs pytest tests |
| **Format code** | `uv run black .` | Formats all Python files |
| **Sort imports** | `uv run isort .` | Sorts and groups imports |
| **Lint code** | `uv run ruff check .` | Checks for code issues |
| **Fix linting** | `uv run ruff check --fix .` | Auto-fixes linting issues |

## Environment Setup

### New Project Workflow
```bash
# 1. Create new project
uv init my-project
cd my-project

# 2. Add core development dependencies
uv add --dev pytest black isort ruff mypy

# 3. Add runtime dependencies
uv add requests fastapi

# 4. Activate virtual environment (optional, uv run handles it)
source .venv/bin/activate  # Linux/Mac
# or
.venv\Scripts\activate     # Windows
```

### Existing Project Setup
```bash
# 1. Clone and navigate to project
git clone https://github.com/user/project.git
cd project

# 2. Install from pyproject.toml
uv sync

# 3. Install development dependencies
uv sync --dev
```

## Essential Development Commands

### Daily Development
```bash
# Install new dependency
uv add pandas

# Install specific version
uv add "numpy<2.0"

# Run tests with coverage
uv run pytest --cov=src

# Format code
uv run black .

# Sort imports
uv run isort .

# Lint and fix
uv run ruff check --fix .

# Type checking
uv run mypy src/
```

### Dependency Management
```bash
# List installed packages
uv pip list

# Update dependencies
uv lock --upgrade

# Remove dependency
uv remove requests

# Export requirements.txt (if needed)
uv pip compile pyproject.toml -o requirements.txt
```

## Pre-commit Configuration

Create `.pre-commit-config.yaml`:
```yaml
repos:
  - repo: https://github.com/astral-sh/ruff-pre-commit
    rev: v0.1.6
    hooks:
      - id: ruff
        args: [--fix]
      - id: ruff-format

  - repo: https://github.com/pre-commit/pre-commit-hooks
    rev: v4.5.0
    hooks:
      - id: trailing-whitespace
      - id: end-of-file-fixer
      - id: check-yaml
      - id: check-added-large-files
```

Install pre-commit hooks:
```bash
uv add --dev pre-commit
uv run pre-commit install
```

## IDE Configuration

### VS Code settings (`.vscode/settings.json`):
```json
{
    "python.defaultInterpreterPath": "./.venv/bin/python",
    "python.terminal.activateEnvironment": true,
    "python.formatting.provider": "black",
    "python.linting.enabled": true,
    "python.linting.ruffEnabled": true,
    "python.sortImports.args": ["--profile", "black"],
    "editor.formatOnSave": true,
    "editor.codeActionsOnSave": {
        "source.organizeImports": true
    }
}
```

## Common Patterns

### Running Different Python Versions
```bash
# Use specific Python version for project
uv python pin 3.11

# Install different Python version
uv python install 3.12

# Run with specific version
uv run --python 3.12 python script.py
```

### Testing with Different Configurations
```bash
# Run specific test file
uv run pytest tests/test_specific.py

# Run with verbose output
uv run pytest -v

# Run with coverage
uv run pytest --cov=src --cov-report=html

# Run specific test marker
uv run pytest -m "slow"
```

## Environment Variables

Use `.env` file for local development:
```bash
# Create .env file
echo "DEBUG=true" > .env
echo "DATABASE_URL=sqlite:///dev.db" >> .env

# Load environment variables
uv run python -c "import os; print(os.getenv('DEBUG'))"
```

## Production Deployment

### Building for Distribution
```bash
# Build wheel and source distribution
uv build

# Install from built distribution
uv pip install dist/project-0.1.0-py3-none-any.whl
```

### Docker Integration
```dockerfile
FROM python:3.11-slim

# Install uv
COPY --from=ghcr.io/astral-sh/uv:latest /uv /bin/uv

# Copy project files
COPY . /app
WORKDIR /app

# Install dependencies
RUN uv sync --frozen

# Run application
CMD ["uv", "run", "python", "main.py"]
```