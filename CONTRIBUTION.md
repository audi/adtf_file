Contribution
============

- [Code guidelines](#code-guidelines)
- [Feature request](#feature-request)
- [Development](#development)

[cpp core guideline]: https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines

[google python style guide]: https://github.com/google/styleguide/blob/gh-pages/pyguide.md

[clean code]: https://medium.com/mindorks/how-to-write-clean-code-lessons-learnt-from-the-clean-code-robert-c-martin-9ffc7aef870c

[meaningful variable names]: https://medium.com/coding-skills/clean-code-101-meaningful-names-and-functions-bf450456d90c

[commit message template]: .github/.gitmessage_template

## Code guidelines <a name="code-guidelines"></a>

* Follow the C++ Core Guidelines [cpp core guideline]
* Try to follow PEP guidelines and [google python style guide]
* Try to follow the SOLID principles of [clean code]
* Use [meaningful variable names]
* For most important commits use [commit message template]

## Feature request <a name="feature-request"></a>

1. Open issue with full feature description

2. Wait for response from core team

## Development <a name="development"></a>

1. Open an issue

2. Create new branch and code there

3. Commit your work using [commit message template]

4. Be sure that you are not behind the master, if so, rebase your branch

5. Make a pull request to master branch with meaningful description and sign the CLA which is part of the PR checks

6. Wait for review

7. Apply / argue the code review comments

8. Clean up the comment history if needed

9. Push the changes again

10. Repeat steps 4-10 until convergence

Side note: **DO NOT USE** `git merge master` or `git pull master` to align your
branch with master, use `git rebase master` for it!

