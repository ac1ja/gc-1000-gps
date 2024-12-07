import os
import git


# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Dynamic Stuff ------------

# Get the current Read the Docs version (branch/tag)
rtd_version = os.environ.get("READTHEDOCS_GIT_IDENTIFIER", "main")
if rtd_version in ["latest", "stable"]:
    github_branch = "main"
else:
    github_branch = rtd_version


# -- Project information -----------------------------------------------------

project = "GC-1000-GPS"
copyright = "2021, To Be Announced"
author = "Nick Soggu, Joe Sedutto"

# The full version, including alpha/beta/rc tags
repo = git.Repo(search_parent_directories=True)
release = str(repo.git.describe("--tags"))


# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinxcontrib.mermaid",
    "sphinx.ext.autosectionlabel",
    "sphinx_tabs.tabs",
]

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]


# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

# Fix missing edit on github button
html_context = {
    "display_github": True,
    "github_user": "ac1ja",
    "github_repo": "gc-1000-gps",
    "github_version": github_branch,
    "conf_py_path": "/docs/",
}

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]


# -- Options for PDF output -------------------------------------------------

latex_elements = {"extraclassoptions": "openany,oneside"}
