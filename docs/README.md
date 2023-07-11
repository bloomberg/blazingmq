# Overview

Use [Just the Docs](https://github.com/just-the-docs/just-the-docs) for documentation

## License

Uses [Just the Docs](https://github.com/just-the-docs/just-the-docs)

# Setup for developing locally (Mac)

## Requirements

### Homebrew

Installed via the bootstrapper tool.

### Ruby 3.1

Installed with Homebrew. https://mac.install.guide/ruby/13.html

Check Ruby is installed correctly with
```
ruby -v
```


## Install dependencies from artifactory

Install the dependencies listed in the Gemfile using the command:
```
bundle install
```

## Building locally

```
bundle exec jekyll build
```

OR

Run the VSCode task 'Build'

## Running locally

```
bundle exec jekyll serve --livereload
```

OR

Run the VSCode task 'Serve'

# Setup for developing locally (Windows)

https://jekyllrb.com/docs/installation/windows/

# Setup for developing locally (Linux)

TBD

## Useful Resources

* https://jekyllrb.com/docs/installation/macos/
