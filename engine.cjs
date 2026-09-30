'use strict';
const build = require('./engine.jsc.meta.json');
if (process.versions.electron !== build.electron || process.platform !== build.platform || process.arch !== build.arch) throw new Error('Incompatible application runtime. Reinstall this version of the app.');
require('bytenode');
module.exports = require('./engine.jsc');
