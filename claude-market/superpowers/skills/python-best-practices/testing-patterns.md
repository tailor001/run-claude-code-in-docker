# Python Testing Patterns and Strategies

## Quick Reference

| Pattern | Description | When to Use |
|---------|-------------|-------------|
| **Unit tests** | Test individual functions/classes in isolation | Always for business logic |
| **Integration tests** | Test multiple components together | When components interact |
| **End-to-end tests** | Test complete user workflows | Critical user paths |
| **Property-based tests** | Generate test cases automatically | Data validation, algorithms |
| **Contract tests** | Test interface compliance | APIs between services |

## Essential pytest Configuration

### pyproject.toml
```toml
[tool.pytest.ini_options]
minversion = "6.0"
addopts = [
    "-ra",  # Show extra test summary info for all tests except passes
    "--strict-markers",  # Raise error for unknown markers
    "--strict-config",   # Raise error for invalid config
    "--cov=src",         # Enable coverage
    "--cov-report=term-missing",  # Show missing lines
    "--cov-report=html", # HTML coverage report
    "--cov-fail-under=80",  # Fail if coverage below 80%
]
testpaths = ["tests"]
python_files = ["test_*.py", "*_test.py"]
python_classes = ["Test*"]
python_functions = ["test_*"]
markers = [
    "slow: marks tests as slow (deselect with '-m \"not slow\"')",
    "integration: marks tests as integration tests",
    "unit: marks tests as unit tests",
    "e2e: marks tests as end-to-end tests",
    "smoke: marks smoke tests that must always pass",
]
```

## Testing Patterns

### 1. AAA Pattern (Arrange-Act-Assert)
```python
import pytest
from calculator import Calculator

def test_calculator_addition():
    """Test calculator addition using AAA pattern."""
    # Arrange
    calculator = Calculator()
    a, b = 5, 3

    # Act
    result = calculator.add(a, b)

    # Assert
    assert result == 8
    assert isinstance(result, (int, float))
```

### 2. Parametrized Tests
```python
import pytest

@pytest.mark.parametrize(
    "input_data, expected_output",
    [
        ("hello", "olleh"),
        ("", ""),
        ("a", "a"),
        ("racecar", "racecar"),  # palindrome
        ("Python", "nohtyP"),
    ],
)
def test_string_reversal(input_data, expected_output):
    """Test string reversal with multiple inputs."""
    from utils import reverse_string
    assert reverse_string(input_data) == expected_output

# Parametrize with multiple parameters
@pytest.mark.parametrize(
    "x, y, operation, expected",
    [
        (5, 3, "add", 8),
        (5, 3, "subtract", 2),
        (5, 3, "multiply", 15),
        (6, 3, "divide", 2),
    ],
)
def test_calculator_operations(x, y, operation, expected):
    """Test calculator with different operations."""
    calculator = Calculator()
    result = getattr(calculator, operation)(x, y)
    assert result == expected
```

### 3. Fixture Patterns
```python
import pytest
from pathlib import Path
from tempfile import TemporaryDirectory

# Simple fixture
@pytest.fixture
def sample_user_data():
    """Provide sample user data for tests."""
    return {
        "id": 1,
        "name": "John Doe",
        "email": "john@example.com",
        "active": True,
    }

# Fixture with setup/teardown
@pytest.fixture
def temporary_file():
    """Create a temporary file for testing."""
    with TemporaryDirectory() as temp_dir:
        file_path = Path(temp_dir) / "test.txt"
        file_path.write_text("test content")
        yield file_path
    # Cleanup happens automatically with context manager

# Factory fixture
@pytest.fixture
def user_factory():
    """Factory for creating test users."""
    def _create_user(name=None, email=None, active=True):
        return {
            "name": name or "Test User",
            "email": email or "test@example.com",
            "active": active,
            "created_at": datetime.utcnow(),
        }
    return _create_user

# Using fixtures
def test_user_processing(sample_user_data):
    """Test user processing with sample data."""
    processor = UserProcessor()
    result = processor.process(sample_user_data)
    assert result["processed"] is True

def test_user_with_factory(user_factory):
    """Test user creation using factory."""
    user = user_factory(name="Alice", email="alice@test.com")
    assert user["name"] == "Alice"
    assert user["active"] is True
```

### 4. Mock and Patch Patterns
```python
import pytest
from unittest.mock import Mock, patch, MagicMock

# Mock external dependencies
def test_api_call_with_mock():
    """Test API call with mocked response."""
    # Arrange
    mock_response = Mock()
    mock_response.status_code = 200
    mock_response.json.return_value = {"result": "success"}

    with patch('requests.get', return_value=mock_response) as mock_get:
        # Act
        result = fetch_data("https://api.example.com")

        # Assert
        assert result["result"] == "success"
        mock_get.assert_called_once_with("https://api.example.com")

# Mock database operations
@patch('database.get_connection')
def test_user_creation(mock_connection):
    """Test user creation with mocked database."""
    # Setup mock
    mock_cursor = Mock()
    mock_cursor.execute.return_value = 1
    mock_cursor.fetchone.return_value = {"id": 123, "name": "Test User"}
    mock_connection.return_value.cursor.return_value = mock_cursor

    # Test
    user = create_user("Test User", "test@example.com")
    assert user["id"] == 123
    assert user["name"] == "Test User"

    # Verify database call
    mock_cursor.execute.assert_called_once()

# Mock specific methods
def test_service_with_partial_mock():
    """Test service with partial mocking."""
    service = ExternalService()

    # Mock only the expensive external call
    with patch.object(service, 'fetch_external_data', return_value="mocked_data"):
        result = service.process_data("input")
        assert result == "processed: mocked_data"
```

### 5. Exception Testing
```python
import pytest

def test_division_by_zero():
    """Test that division by zero raises proper exception."""
    calculator = Calculator()

    with pytest.raises(ZeroDivisionError, match="division by zero"):
        calculator.divide(10, 0)

def test_invalid_input_validation():
    """Test custom validation exceptions."""
    validator = EmailValidator()

    with pytest.raises(ValidationError) as exc_info:
        validator.validate("invalid-email")

    assert "Invalid email format" in str(exc_info.value)
    assert exc_info.value.field == "email"

def test_timeout_handling():
    """Test timeout exception handling."""
    with pytest.raises(TimeoutError):
        perform_slow_operation(timeout=0.001)
```

### 6. Property-Based Testing
```python
import hypothesis
from hypothesis import given, strategies as st

@given(st.text(min_size=1, max_size=100))
def test_string_operations_commutation(text):
    """Test that string operations have expected properties."""
    # Property: Reversing twice returns original
    assert text == reverse_string(reverse_string(text))

    # Property: Length is preserved in reversal
    assert len(text) == len(reverse_string(text))

@given(st.integers(min_value=0, max_value=1000), st.integers(min_value=0, max_value=1000))
def test_commutative_addition(a, b):
    """Test that addition is commutative."""
    assert a + b == b + a

@given(st.lists(st.integers()))
def test_sort_properties(numbers):
    """Test properties of sorting."""
    sorted_numbers = sorted(numbers)

    # Property: Sorted list has same length
    assert len(sorted_numbers) == len(numbers)

    # Property: Sorted list is actually sorted
    assert all(sorted_numbers[i] <= sorted_numbers[i + 1]
               for i in range(len(sorted_numbers) - 1))

    # Property: Sorted list contains same elements (multiset equality)
    assert sorted(numbers) == sorted_numbers
```

## Test Organization

### Directory Structure
```
tests/
├── unit/                   # Unit tests
│   ├── test_models.py
│   ├── test_services.py
│   └── test_utils.py
├── integration/            # Integration tests
│   ├── test_database.py
│   └── test_api.py
├── e2e/                   # End-to-end tests
│   ├── test_user_flows.py
│   └── test_workflows.py
├── conftest.py            # Shared fixtures
├── __init__.py
└── helpers/               # Test helpers and utilities
    ├── fixtures.py
    └── factories.py
```

### Conftest.py for Shared Fixtures
```python
import pytest
from pathlib import Path
import tempfile
import shutil

@pytest.fixture(scope="session")
def test_data_dir():
    """Provide test data directory path."""
    return Path(__file__).parent / "data"

@pytest.fixture(scope="function")
def temp_directory():
    """Create temporary directory that's cleaned up after test."""
    temp_dir = Path(tempfile.mkdtemp())
    yield temp_dir
    shutil.rmtree(temp_dir)

@pytest.fixture(scope="session")
def test_database():
    """Set up test database for session."""
    # Setup
    db = create_test_database()
    yield db
    # Cleanup
    db.cleanup()

# Markers for different test types
def pytest_configure(config):
    config.addinivalue_line(
        "markers", "slow: marks tests as slow (deselect with '-m \"not slow\"')"
    )
    config.addinivalue_line(
        "markers", "integration: marks tests as integration tests"
    )
```

## Coverage and Quality Metrics

### Running Tests with Coverage
```bash
# Run all tests with coverage
uv run pytest

# Run specific test files with coverage
uv run pytest tests/unit/test_models.py --cov=src/models

# Generate coverage HTML report
uv run pytest --cov=src --cov-report=html

# Coverage with branch checking
uv run pytest --cov=src --cov-branch

# Coverage for specific module only
uv run pytest --cov=src.services --cov-report=term-missing
```

### Quality Gates
```bash
# Run tests and fail if coverage below threshold
uv run pytest --cov=src --cov-fail-under=80

# Run tests with strict markers (fail on unknown markers)
uv run pytest --strict-markers

# Run tests with xfail (expected failures) strictness
uv run pytest --xfail --strict
```

## Performance Testing

### Benchmark Testing with pytest-benchmark
```python
import pytest
from algorithms import quick_sort, merge_sort

def test_sorting_performance(benchmark):
    """Benchmark sorting algorithms."""
    data = list(range(1000, 0, -1))  # Reverse sorted list

    result = benchmark(quick_sort, data)
    assert len(result) == 1000
    assert result == sorted(data)

@pytest.mark.parametrize("algorithm", [quick_sort, merge_sort])
def test_sort_comparison(benchmark, algorithm):
    """Compare different sorting algorithms."""
    data = list(range(1000))
    result = benchmark(algorithm, data.copy())
    assert result == sorted(data)
```

## Advanced Patterns

### Test Factories
```python
class UserFactory:
    """Factory for creating test users."""

    @staticmethod
    def create(**overrides):
        """Create user with optional field overrides."""
        defaults = {
            "id": 1,
            "name": "Test User",
            "email": "test@example.com",
            "active": True,
        }
        return {**defaults, **overrides}

def test_user_with_factory():
    """Test using factory pattern."""
    user = UserFactory.create(name="Alice", active=False)
    assert user["name"] == "Alice"
    assert user["active"] is False
    assert user["email"] == "test@example.com"  # Default value
```

### Test Utilities
```python
# tests/helpers/assertions.py
def assert_valid_response(response, expected_status=200):
    """Common response validation."""
    assert response.status_code == expected_status
    assert "application/json" in response.headers.get("content-type", "")
    return response.json()

def assert_user_data(actual, expected):
    """Assert user data structure."""
    required_fields = ["id", "name", "email"]
    for field in required_fields:
        assert field in actual, f"Missing field: {field}"
        assert actual[field] == expected[field], f"Field mismatch: {field}"

# Using custom assertions
def test_api_response():
    response = client.get("/api/users/1")
    data = assert_valid_response(response)
    assert_user_data(data, {"id": 1, "name": "John", "email": "john@test.com"})
```