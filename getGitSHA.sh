#!/bin/sh
# Print additional version information for non-release trees.

usage() {
	echo "Usage: $0 [srctree]" >&2
	exit 1
}

cd "${1:-.}" || usage

# Check for git and a git repo.
#if head=`git rev-parse --verify HEAD 2>/dev/null`; then
#	printf '%s%s' -g `echo "$head" | cut -c1-8`
#
#	# Are there uncommitted changes?
#	if git diff-files | read dummy; then
#		printf '%s' -dirty
#	fi
#else
#	printf '%s' -nogit
#fi

if head=`git rev-parse --verify HEAD 2>/dev/null`; then

    # Are there uncommitted changes?
    if git diff-files | read dummy; then
		printf '%s' 00000000
	else
        printf '%s' `echo "$head" | cut -c1-8`
    fi
else
	printf '%s' 00000000
fi
