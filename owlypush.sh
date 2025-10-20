
#!/bin/bash

git add .

if [ -z "$1" ]; then
    msg="Minor hoot"
else
    msg="$1"
fi

if git diff --cached --quiet; then
    echo "No changes to commit"
else
    git commit -m "$msg"
fi

# Only rename branch if still on 'master'
current_branch=$(git symbolic-ref --short HEAD)

if [ "$current_branch" = "master" ]; then
    git branch -m main
    current_branch = "main"
fi

# Pull changes to avoid non-fast-forward error
git pull origin "$current_branch" --rebase

git push origin "$current_branch"
