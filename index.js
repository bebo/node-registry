var Native = require('bindings')('node-registry');
const util = require('util');
var promisify = util.promisify;

if (promisify == null) {
  const Promise = require('bluebird');
  promisify = Promise.promisify;
}

var Registry = {
};


Registry.getValue = promisify(Native.getValue);
Registry.putValue = promisify(Native.putValue);
Registry.deleteValue = promisify(Native.deleteValue);

module.exports = Registry;

