# Overview

Use [Just the Docs](https://github.com/just-the-docs/just-the-docs) for documentation

## License

In order to build the documentation, we use the [Just the
Docs](https://github.com/just-the-docs/just-the-docs) theme for Jekyll, which
is licensed under the [MIT license](../licenses/LICENSE-just-the-docs.txt).
Please see that file for the theme’s copyright and permission notices.

# Setup for developing locally (Mac)

## Requirements

### Homebrew

Install [Homebrew](https://brew.sh/).
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

### Ruby 3.1

Installed with Homebrew. https://mac.install.guide/ruby/13.html

Check Ruby is installed correctly with
```
ruby -v
```


## Install dependencies

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
