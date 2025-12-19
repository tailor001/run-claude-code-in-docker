# Code Review Checklist

## Code Quality
- [ ] **Code follows PEP 8** and passes black formatting
- [ ] **Imports are sorted** and organized correctly
- [ ] **No linting warnings** from ruff
- [ ] **Type hints** are used consistently and correctly
- [ ] **Docstrings** follow Google/NumPy style conventions
- [ ] **Variable and function names** are descriptive and follow Python conventions
- [ ] **Complex functions** are broken down into smaller, testable pieces

## Architecture and Design
- [ ] **Single Responsibility Principle** is followed
- [ ] **Dependencies** are properly injected rather than hardcoded
- [ ] **Separation of concerns** between business logic and infrastructure
- [ ] **Error handling** is appropriate and consistent
- [ ] **Configuration** is externalized, not hardcoded
- [ ] **Circular dependencies** are avoided
- [ ] **Interfaces** are clear and minimal

## Testing
- [ ] **New features** have corresponding tests
- [ ] **Test coverage** meets project threshold (typically >80%)
- [ ] **Tests are well-organized** with clear names and descriptions
- [ ] **Edge cases and error conditions** are tested
- [ ] **Integration tests** verify component interactions
- [ ] **Test fixtures** are used appropriately
- [ ] **Tests are deterministic** and don't rely on external state

## Security
- [ ] **Input validation** is implemented for all user inputs
- [ ] **SQL injection** prevention is used (parameterized queries)
- [ ] **Authentication and authorization** are properly implemented
- [ ] **Sensitive data** is properly handled and not logged
- [ ] **Dependencies** are up-to-date and free from known vulnerabilities
- [ ] **Error messages** don't leak sensitive information
- [ ] **CORS and other security headers** are configured correctly

## Performance
- [ ] **Database queries** are optimized (N+1 problems avoided)
- [ ] **Caching** is used where appropriate
- [ ] **Memory usage** is reasonable (no memory leaks)
- [ ] **Async/await** is used correctly for I/O operations
- [ ] **Background tasks** are used for long-running operations
- [ ] **Connection pooling** is configured for databases/external services
- [ ] **Lazy loading** is used where appropriate

## API Design (if applicable)
- [ ] **HTTP status codes** are used correctly
- [ ] **API endpoints** follow REST conventions
- [ ] **Request/response schemas** are properly defined
- [ ] **Rate limiting** is implemented where needed
- [ ] **API documentation** is accurate and up-to-date
- [ ] **Versioning** strategy is clear
- [ ] **Error responses** are consistent and informative

## Database Operations
- [ ] **Database migrations** are provided for schema changes
- [ ] **Transactions** are used appropriately
- [ ] **Connection management** is proper
- [ ] **Database indexes** are used for query optimization
- [ ] **Data validation** happens at the application level
- [ ] **Database access** is abstracted through repositories/services
- [ ] **Backup and recovery** procedures are documented

## Configuration and Environment
- [ ] **Environment variables** are used for configuration
- [ ] **Default values** are sensible
- [ ] **Configuration validation** is implemented
- [ ] **Development vs production** configurations are properly separated
- [ ] **Secrets management** follows security best practices
- [ ] **Configuration documentation** is complete
- [ ] **Required environment variables** are documented

## Documentation
- [ ] **README is updated** with new features/changes
- [ ] **API documentation** reflects current implementation
- [ ] **Complex algorithms** are well-documented in code
- [ ] **Dependencies and requirements** are clearly stated
- [ ] **Setup and deployment** instructions are accurate
- [ ] **Examples and usage patterns** are provided
- [ ] **Troubleshooting guide** includes common issues

## Python-Specific Best Practices
- [ ] **Context managers** (`with` statements) are used for resource management
- [ ] **List/dict comprehensions** are used appropriately
- [ ] **Property decorators** are used instead of getters/setters
- [ ] **Dataclasses** are used for simple data containers
- [ ] **f-strings** are used for string formatting
- [ ] **Pathlib** is used for file system operations
- [ ] **Enumerations** are used instead of magic strings/numbers

## Tooling and Automation
- [ ] **Pre-commit hooks** will catch any issues
- [ ] **CI/CD pipeline** will pass with these changes
- [ ] **Dependencies** are properly specified in pyproject.toml
- [ ] **Version bumping** is appropriate (semantic versioning)
- [ ] **Changelog** is updated with user-facing changes
- [ ] **Migration scripts** are provided if needed
- [ ] **Rollback procedures** are documented

## Review Process
- [ ] **All automated checks pass** (tests, linting, type checking)
- [ ] **Changes are focused** and address a single concern
- [ ] **Code is self-documenting** and easy to understand
- [ ] **Performance impact** has been considered and measured if needed
- [ ] **Backward compatibility** is maintained or breaking changes are justified
- [ ] **Approval requirements** are met (number of reviewers, domain experts)
- [ ] **Deployment considerations** are documented

## Questions to Ask During Review
- Does this code solve the intended problem effectively?
- Is this the simplest solution that works?
- Are there edge cases that haven't been considered?
- Is this code testable and are the tests comprehensive?
- Will this code be maintainable by other developers?
- Are there security implications that need to be addressed?
- Does this follow established patterns in the codebase?
- Is the performance acceptable for the expected load?