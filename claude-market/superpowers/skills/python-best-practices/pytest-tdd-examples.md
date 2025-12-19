# pytest TDD Examples for Python

**Purpose**: Complete examples of applying test-driven-development RED-GREEN-REFACTOR cycle with pytest and modern Python ecosystem.

## Core pytest-TDD Workflow

### Test Structure and Naming

```python
# Test naming conventions: test_[function]_[expected_behavior]
def test_user_email_validation_should_accept_valid_emails():
    pass

# Use descriptive test names that explain the behavior
def test_user_password_hash_should_be_different_from_plain_password():
    pass

# Group related tests in classes
class TestUserAuthentication:
    def test_should_accept_valid_credentials(self):
        pass

    def test_should_reject_invalid_password(self):
        pass
```

## Complete TDD Examples

### Example 1: User Authentication System

#### RED Phase - Write Failing Tests
```python
# tests/test_user.py
import pytest
from user import User

class TestUserCreation:
    def test_user_creation_should_store_email_hashed_password(self):
        user = User("alice@example.com", "password123")

        assert user.email == "alice@example.com"
        assert user.password_hash != "password123"  # Should be hashed
        assert len(user.password_hash) == 60  # bcrypt hash length

    def test_user_creation_should_reject_invalid_email(self):
        with pytest.raises(ValueError, match="Invalid email"):
            User("invalid-email", "password123")

class TestUserAuthentication:
    def test_authentication_should_succeed_with_correct_password(self):
        user = User("alice@example.com", "password123")
        assert user.authenticate("password123") is True

    def test_authentication_should_fail_with_wrong_password(self):
        user = User("alice@example.com", "password123")
        assert user.authenticate("wrongpassword") is False

    def test_authentication_should_fail_for_none_password(self):
        user = User("alice@example.com", "password123")
        assert user.authenticate(None) is False
```

#### GREEN Phase - Minimal Implementation
```python
# user.py
import bcrypt

class User:
    def __init__(self, email, password):
        if "@" not in email:
            raise ValueError("Invalid email")
        self.email = email
        # Always return same hash - passes tests but is wrong
        self.password_hash = "$2b$12$fixedhashthatis60charslonggggggggggggggggggggggg"

    def authenticate(self, password):
        if password == "password123":
            return True
        return False
```

#### REFACTOR Phase - Real Implementation
```python
# user.py
import bcrypt
import re
from typing import Optional

class User:
    def __init__(self, email: str, password: str):
        self._validate_email(email)
        self.email = email.lower().strip()
        self.password_hash = self._hash_password(password)

    def _validate_email(self, email: str) -> None:
        """Validate email format."""
        if not email or not isinstance(email, str):
            raise ValueError("Invalid email")

        # Simple email validation
        pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
        if not re.match(pattern, email):
            raise ValueError("Invalid email")

    def _hash_password(self, password: str) -> str:
        """Hash password using bcrypt."""
        if not password or not isinstance(password, str):
            raise ValueError("Password must be a non-empty string")

        # Generate salt and hash
        salt = bcrypt.gensalt()
        return bcrypt.hashpw(password.encode('utf-8'), salt).decode('utf-8')

    def authenticate(self, password: Optional[str]) -> bool:
        """Verify password against stored hash."""
        if password is None:
            return False

        try:
            return bcrypt.checkpw(
                password.encode('utf-8'),
                self.password_hash.encode('utf-8')
            )
        except (ValueError, TypeError):
            return False

    def change_password(self, old_password: str, new_password: str) -> None:
        """Change user password after verifying old password."""
        if self.authenticate(old_password):
            self.password_hash = self._hash_password(new_password)
        else:
            raise ValueError("Invalid current password")
```

### Example 2: Data Processing Pipeline

#### RED Phase
```python
# tests/test_data_processor.py
import pytest
from data_processor import DataProcessor

class TestDataProcessor:
    def setup_method(self):
        """Setup before each test."""
        self.processor = DataProcessor()

    def test_process_empty_list_should_return_empty(self):
        result = self.processor.process_numbers([])
        assert result == []

    def test_process_list_with_positive_numbers(self):
        result = self.processor.process_numbers([1, 2, 3, 4, 5])
        assert result == [1, 4, 9, 16, 25]

    def test_process_list_with_negative_numbers(self):
        result = self.processor.process_numbers([-1, -2, -3])
        assert result == [1, 4, 9]

    def test_process_list_with_mixed_numbers(self):
        result = self.processor.process_numbers([2, -3, 4])
        assert result == [4, 9, 16]

    def test_process_list_with_zero(self):
        result = self.processor.process_numbers([0, 1, 2])
        assert result == [0, 1, 4]

    def test_process_should_reject_non_numeric_data(self):
        with pytest.raises(TypeError):
            self.processor.process_numbers([1, 2, "three"])

    def test_process_with_custom_filter(self):
        def even_only(x):
            return x % 2 == 0

        result = self.processor.process_numbers([1, 2, 3, 4, 5], filter_func=even_only)
        assert result == [4, 16]  # Only squares of even numbers
```

#### GREEN Phase
```python
# data_processor.py - Minimal implementation
from typing import List, Callable, Optional

class DataProcessor:
    def process_numbers(self, numbers: List[float], filter_func: Optional[Callable] = None) -> List[float]:
        # Minimal implementation - just return squares of first 3 items
        result = []
        for i, num in enumerate(numbers[:3]):
            if filter_func is None or filter_func(num):
                result.append(num * num)
        return result
```

#### REFACTOR Phase
```python
# data_processor.py - Complete implementation
from typing import List, Callable, Optional, Union

class DataProcessor:
    """Process numeric data with various transformations and filters."""

    def __init__(self):
        self.processed_count = 0

    def process_numbers(
        self,
        numbers: List[Union[int, float]],
        filter_func: Optional[Callable[[Union[int, float]], bool]] = None,
        transform_func: Optional[Callable[[Union[int, float]], Union[int, float]]] = None
    ) -> List[Union[int, float]]:
        """
        Process a list of numbers applying optional filter and transform functions.

        Args:
            numbers: List of numbers to process
            filter_func: Function to filter numbers (return True to keep)
            transform_func: Function to transform numbers (default: square)

        Returns:
            List of processed numbers

        Raises:
            TypeError: If any element in numbers is not numeric
        """
        # Validate input
        for num in numbers:
            if not isinstance(num, (int, float)):
                raise TypeError(f"All elements must be numeric, got {type(num)}")

        # Default transform is square
        if transform_func is None:
            transform_func = lambda x: x * x

        # Process numbers
        result = []
        for num in numbers:
            if filter_func is None or filter_func(num):
                processed = transform_func(num)
                result.append(processed)
                self.processed_count += 1

        return result

    def get_statistics(self) -> dict:
        """Get processing statistics."""
        return {
            "processed_count": self.processed_count
        }

    def reset_statistics(self) -> None:
        """Reset processing statistics."""
        self.processed_count = 0
```

## Advanced Testing Patterns

### Property-Based Testing with Hypothesis
```python
# test_with_hypothesis.py
from hypothesis import given, strategies as st
from user import User

@given(st.emails(), st.text(min_size=8))
def test_user_auth_properties(email, password):
    """Property-based test for user authentication."""
    user = User(email, password)

    # Authentication should always work with original password
    assert user.authenticate(password) is True

    # Email should always be normalized to lowercase
    assert user.email == email.lower()

@given(st.lists(st.integers(min_value=1, max_value=1000)))
def test_data_processor_properties(numbers):
    """Property-based test for data processor."""
    from data_processor import DataProcessor
    processor = DataProcessor()

    result = processor.process_numbers(numbers)

    # Result should have same length as input
    assert len(result) == len(numbers)

    # All results should be non-negative
    assert all(x >= 0 for x in result)

    # Each result should be the square of corresponding input
    for input_val, output_val in zip(numbers, result):
        assert output_val == input_val ** 2
```

### Fixtures and Parametrization
```python
# conftest.py - Shared fixtures
import pytest
from user import User
from data_processor import DataProcessor

@pytest.fixture
def sample_user():
    """Create a sample user for testing."""
    return User("test@example.com", "password123")

@pytest.fixture
def data_processor():
    """Create a fresh data processor for each test."""
    return DataProcessor()

@pytest.fixture
def user_data():
    """Sample user data for tests."""
    return [
        ("alice@example.com", "alice123"),
        ("bob@example.com", "bob456"),
        ("charlie@example.com", "charlie789")
    ]

# Parametrized tests
@pytest.mark.parametrize("email,password,expected", [
    ("user@example.com", "validpass123", True),
    ("user@example.com", "invalidpass", False),
    ("USER@EXAMPLE.COM", "validpass123", True),  # Case insensitive
])
def test_authentication_with_various_inputs(email, password, expected):
    user = User(email, "validpass123")
    assert user.authenticate(password) is expected
```

### Mocking Best Practices
```python
# test_api_client.py - Good mocking examples
import pytest
from unittest.mock import Mock, patch
from api_client import APIClient

class TestAPIClient:
    def test_fetch_data_success(self):
        """Test with test server instead of mock."""
        with TestServer() as server:
            server.add_response("/data", {"status": "ok", "data": [1, 2, 3]})
            client = APIClient(server.url)
            result = client.fetch_data("/data")
            assert result["data"] == [1, 2, 3]

    def test_fetch_data_network_error(self):
        """Test network error handling."""
        with patch('requests.get') as mock_get:
            mock_get.side_effect = ConnectionError("Network error")
            client = APIClient("http://api.example.com")

            with pytest.raises(APIError):
                client.fetch_data("/data")

# Test double implementation
class TestServer:
    """Simple test server for API testing."""
    def __init__(self):
        self.responses = {}
        self.url = "http://test-server"

    def add_response(self, path, data):
        self.responses[path] = data

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

# API client updated to use dependency injection
class APIClient:
    def __init__(self, base_url, session=None):
        self.base_url = base_url
        self.session = session or requests.Session()

    def fetch_data(self, endpoint):
        """Fetch data from API."""
        response = self.session.get(f"{self.base_url}{endpoint}")
        response.raise_for_status()
        return response.json()
```

## Integration with CI/CD

### pytest Configuration
```ini
# pytest.ini
[tool:pytest]
testpaths = tests
python_files = test_*.py
python_classes = Test*
python_functions = test_*
addopts =
    --strict-markers
    --strict-config
    --verbose
    --cov=src
    --cov-report=term-missing
    --cov-report=html
    --cov-fail-under=80
markers =
    slow: marks tests as slow (deselect with '-m "not slow"')
    integration: marks tests as integration tests
    unit: marks tests as unit tests
```

### GitHub Actions Workflow
```yaml
# .github/workflows/test.yml
name: Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        python-version: [3.9, 3.10, 3.11]

    steps:
    - uses: actions/checkout@v3

    - name: Set up Python
      uses: actions/setup-python@v4
      with:
        python-version: ${{ matrix.python-version }}

    - name: Install dependencies
      run: |
        python -m pip install --upgrade pip
        pip install -r requirements.txt
        pip install -r requirements-dev.txt

    - name: Run pytest
      run: |
        pytest --cov=src --cov-report=xml

    - name: Upload coverage
      uses: codecov/codecov-action@v3
```

## Integration with Superpowers 4.0.0

When writing Python tests:
1. **test-driven-development**: Always write failing test first
2. **systematic-debugging**: If tests fail unexpectedly
3. **Use pytest fixtures**: For clean test setup/teardown
4. **Property-based testing**: With hypothesis for edge cases
5. **verification-before-completion**: Ensure all tests pass before commit

This creates robust, well-tested Python code with comprehensive coverage.