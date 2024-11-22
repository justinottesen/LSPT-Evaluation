# Developer Instructions

## Setup, Building, & Running
### Dependencies

- cmake (minimum version 3.13)
- make (minimum version ____ <--- FILL IN VERSION HERE WHEN KNOWN)
- clang (must support c++20 and clang-format ____ <--- FILL IN VERSION HERE WHEN KNOWN)
- python3 (minimum version 3.____ <--- FILL IN VERSION HERE WHEN KNOWN)

### Upon Cloning the Repository

Begin by running the `setup_repo.sh` script. This will do more in the future. Currently it does the following:

1. Clones the googletest repository if it is not present
2. Builds the libraries from the googletest source

### Build & Run Instructions

#### Evaluation

Enter the evaluation directory, and run make. This will create a binary titled `evaluation` in the `evaluation/bin` directory which can be executed:

```
cd evaluation
make
./bin/evaluation
```

Alternatively, the `make run` shortcut will also do this

#### Evaltool

Currently this is python, and it is set up with the shebang, so all that is necessary should be `./evaltool/evaltool.py <arguments>`. This may change if we choose to use C++.

#### Unit Tests

Enter the `unittest` directory, and run `make`. By default, this will make all unit tests, linking with the google test libraries built by `setup_repo.sh`.

**IF YOU GET ERRORS HERE, IT IS PROBABLY BECAUSE YOU DIDN'T RUN THE SETUP SCRIPT**

Or maybe I set it up wrong, still a work in progress

#### Component Tests

Enter the `componenttest` directory and run `pytest`. This should run everything.

## Collaboration

Below is the general order of steps for development:
1. Use the [GitHub Project](https://github.com/users/justinottesen/projects/4) pick up or create an issue to work on (Make sure the issue is also in the [issues page](https://github.com/justinottesen/LSPT-Evaluation/issues) of the repository)
2. Create a branch (and link it to the issue), and complete the job associated with the issue
3. Run the test suite to ensure the changes do not cause problems
4. Create a Pull Request. It will need two approvals, as well as a fully passing test run in order to merge to `master`. No tooling is in place to ensure test coverage of written code, this will be at the discretion of the team members who approve a PR.
5. Merge the pull request once approved and all checks pass.

### Branching Strategy

Our branching strategy will be incredibly simple. With a team of only 4, we will create branches off of master, and merge directly to master. We may consider feature branches in the future, but we do not forsee this being immediately necessary.

### Coding Styles & Conventions

There is a `.clang-format` file in the `Code` directory. This should be used to format all C++ code. In the future, there may also be `clang-tidy` checks to enforce best practices, however the compiler will also be set to `-Wall -Wextra -Werror`.

Python has no tooling set up in this repository. In general, we will try to just follow what we think is right, and match surrounding code.
> Maybe use flake8 and/or black?

## Helpful Resources

- [GoogleTest User's Guide](https://google.github.io/googletest/)
- [Pytest Documentation](https://docs.pytest.org/en/stable/index.html)
- [Makefile Tutorial](https://makefiletutorial.com/)
