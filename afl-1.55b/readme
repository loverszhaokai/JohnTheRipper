# subdir

Return whether a path is a sub-path.

This module doesn't do any IO, only pure path computations.

[![browser support](http://ci.testling.com/substack/subdir.png)](http://ci.testling.com/substack/subdir)

[![build status](https://secure.travis-ci.org/substack/subdir.png)](http://travis-ci.org/substack/subdir)

# example

``` js
var subdir = require('subdir');

console.dir([
    subdir(__dirname, './inside.js'),
    subdir(__dirname, '../../subdir/example/inside.js'),
    subdir(__dirname, '../index.js')
]);
```

```
$ node example/inside.js
[ true, true, false ]
```

# methods

``` js
var subdir = require('subdir')
```

## subdir(parent, path)

Return a boolean value: whether the `path` is a subdirectory of `parent`.

`path` is resolved with respect to `parent` if it is a relative path.

# install

With [npm](https://npmjs.org) do:

```
npm install subdir
```

# license

MIT