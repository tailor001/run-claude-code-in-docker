# Project Setup Checklist

## Environment Setup
- [ ] **Initialize project with uv**: `uv init project-name`
- [ ] **Navigate to project**: `cd project-name`
- [ ] **Create virtual environment**: `uv venv`
- [ ] **Add core dependencies**: `uv add fastapi pydantic sqlalchemy`
- [ ] **Add development dependencies**: `uv add --dev pytest black isort ruff mypy pre-commit`
- [ ] **Install pre-commit hooks**: `uv run pre-commit install`

## Project Structure
- [ ] **Create source directory**: `mkdir -p src/project_name`
- [ ] **Create test directories**: `mkdir -p tests/{unit,integration,e2e}`
- [ ] **Create documentation directory**: `mkdir docs`
- [ ] **Create scripts directory**: `mkdir scripts`
- [ ] **Add __init__.py files** to all Python packages
- [ ] **Set up .gitignore** for Python projects

## Configuration Files
- [ ] **Configure pyproject.toml** with project metadata
- [ ] **Set up black configuration** in pyproject.toml
- [ ] **Configure isort** with black profile
- [ ] **Set up ruff configuration** for linting
- [ ] **Configure mypy** for type checking
- [ ] **Set up pytest configuration** with markers and coverage
- [ ] **Create .pre-commit-config.yaml** with quality tools

## Development Tools
- [ ] **Configure VS Code settings** for Python development
- [ ] **Set up debugging configuration** in IDE
- [ ] **Create environment file** (.env) for local development
- [ ] **Set up database** (if applicable)
- [ ] **Configure logging** for development

## Documentation
- [ ] **Create README.md** with project overview
- [ ] **Add installation instructions** using uv
- [ ] **Include development setup** instructions
- [ ] **Document API endpoints** (if applicable)
- [ ] **Add contribution guidelines**

## Testing Setup
- [ ] **Create conftest.py** with shared fixtures
- [ ] **Set up test factories** for test data
- [ ] **Configure coverage** thresholds
- [ ] **Create test database** (if applicable)
- [ ] **Set up test markers** for different test types

## CI/CD Setup
- [ ] **Create GitHub Actions workflow** for testing
- [ ] **Set up automated testing** on push/PR
- [ ] **Configure code quality checks** in CI
- [ ] **Set up coverage reporting**
- [ ] **Configure deployment workflow** (if applicable)

## Security Setup
- [ ] **Add .env.example** with required environment variables
- [ ] **Set up secret management** for production
- [ ] **Configure security scanning** (bandit)
- [ ] **Set up dependency scanning**
- [ ] **Add API rate limiting** (if applicable)

## Performance Setup
- [ ] **Set up application monitoring**
- [ ] **Configure performance testing** (pytest-benchmark)
- [ ] **Set up database connection pooling**
- [ ] **Configure caching** (if applicable)
- [ ] **Set up logging** for performance monitoring

## Production Preparation
- [ ] **Create Dockerfile** with best practices
- [ ] **Set up docker-compose.yml** for local development
- [ ] **Configure health checks**
- [ ] **Set up proper logging** for production
- [ ] **Create deployment scripts**
- [ ] **Set up backup strategies** (if applicable)

## Validation Commands
```bash
# Validate formatting
uv run black --check .

# Validate imports
uv run isort --check-only .

# Validate linting
uv run ruff check .

# Validate type checking
uv run mypy src/

# Run tests with coverage
uv run pytest --cov=src --cov-fail-under=80

# Check security
uv run bandit -r src/

# Validate dependencies
uv sync --frozen
```