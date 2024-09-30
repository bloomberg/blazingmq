import os

def check_pr_title():
    title = os.environ.get("PR_TITLE")
    if title is None or len(title) == 0:
        raise RuntimeError("This script expects a non-empty environment variable PR_TITLE set")
    title = title.lower()
    valid_prefixes = ["fix", "feat", "perf", "ci", "build", "revert", "ut", "it", "docs", "refactor", "misc"]
    if not any(title.startswith(prefix.lower()) for prefix in valid_prefixes):
        raise RuntimeError("PR title \"{}\" doesn't start with a valid prefix, allowed prefixes: {}".format(title, " ".join(valid_prefixes))

if __name__ == "__main__":
    check_pr_title()
