# Contributing

Before contributing to this repository, first please discuss the change you wish to make via an
issue, or any other method of communication with the maintainers of this repository.

You can also search this project for issues with the following labels:

| Label                                                                                                                                       | Search Term                               | Description                                                                                                                                                        |
| ------------------------------------------------------------------------------------------------------------------------------------------- | ----------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| [good-first-issue]https://github.com/search?q=repo%3Abloomberg%2Fblazingmq+is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22&type=issues) | `is:issue is:open label:good first issue` | Recommended for first-time contributors! These are well-defined, and allow a user to get familiar with the project's workflow before tackling more complex issues. |
| [help wanted](https://github.com/search?q=repo%3Abloomberg%2Fblazingmq+is%3Aissue+is%3Aopen+label%3A%22help+wanted%22&type=issues)          | `is:issue is:open label:"help wanted"`    | General issues where contributors help is wanted.                                                                                                                  |
| [A-](https://github.com/bloomberg/blazingmq/labels?q=A)                                                                                     |                                           | Category for the *area* which this issue covers |

## Contribution Licensing

Since this project is distributed under the terms of an [open source license](LICENSE), contributions that
you make are licensed under the same terms. In order for us to be able to accept your contributions,
we will need explicit confirmation from you that you are able and willing to provide them under
these terms, and the mechanism we use to do this is called a Developer's Certificate of Origin
[(DCO)](https://github.com/bloomberg/.github/blob/main/DCO.md). This is very similar to the process
used by the Linux kernel, Samba, and many other major open source projects.

To participate under these terms, all that you must do is include a line like the following as the
last line of the commit message for each commit in your contribution:

    Signed-Off-By: Random J. Developer <random@developer.example.org>

The simplest way to accomplish this is to add `-s` or `--signoff` to your `git commit` command.

You must use your real name (sorry, no pseudonyms, and no anonymous contributions).

## Documentation

**Public** modules, functions, classes, and methods must be documented using [Doxygen][doxygen]. **Non-public** functions and methods must also be documented for defining the
API contract. In addition to being useful for generating documentation, docstrings add clarity when
looking through the source code, and can be leveraged in autocompletion by IDEs.

### Docstring and code style

Docstrings use the triple-slash `///` style comments for docstrings and `@` for special commands. Future contributions should make use of the `@brief`, `@details`, `@params`, and `@returns` directives in the docstrings.

Otherwise much of the BlazingMQ codebase is written using the [BDE style guide.](bde-style)

## Tests

Changes should always include tests. If this is a bug fix it is a good idea to add the tests as the
first commit of the pull request and the changes to fix the issue in subsequent commits to make it
easier to validate it.

## Pull requests

### Linting your code

Before commiting anything, be sure to format any C++ changes:

```shell
git clang-format
```

This will ensure that your contribution passes our linting checks.

### PRs should be linked to a GitHub issue

Before opening a pull request to this repository, first please make sure there is a GitHub issue
where your change has been discussed with the maintainers. Mention the issue number in your pull
request description using one of the
[supported keywords](https://help.github.com/articles/closing-issues-using-keywords/). For example,
adding `Closes: #100` in the pull request description will link the PR with the issue and GitHub
will automatically close the issue upon merging it.

Do not include the issue reference in your commit messages however, add it only in the description
of the PR.

<!-- LINKS -->

[doxygen]: https://www.doxygen.nl/ "Docstring Conventions"
[bde-style]: https://bloomberg.github.io/bde/knowledge_base/coding_standards.html "BDE Code Style Guide"

<!--
vim: tw=99:spell
-->
