var Registry = require('../');
var assert = require('assert');
const Promise = require('bluebird');

describe('registry', function () {

  it('get key', function () {
    return Registry.getValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\GameCapture",
      key: "Logs",
    }).then(function (data) {
      console.error(data);
      return true;
    }).catch(function (err) {
      console.error(err);
      throw err;
    });
  });

  it('put key', function () {
    return Registry.putValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\TestCapture",
      type: "REG_QWORD",
      value: "9223372036854775800",
      key: "Testing12",
    }).then(function (data) {
      console.error(data);
      return true;
    }).catch(function (err) {
      console.error(err);
      throw err;
    });
  });

  it('delete key', function () {
    return Registry.putValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\GameCapture",
      type: "REG_SZ",
      value: "test",
      key: "Testing122232",
    }).then(function (data) {
      return Registry.deleteValue({
        hkey: "HKEY_CURRENT_USER",
        subkey: "SOFTWARE\\Bebo\\GameCapture",
        key: "Testing122232",
      })
    }).then(function (data) {
      console.error(data);
      return true;
    }).catch(function (err) {
      console.error(err);
      throw err;
    });
  });

  it('create key', function () {
    return Registry.createValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\GameCapture",
      type: "REG_SZ",
      value: "test",
      key: "Testing122232",
    }).then(function (data) {
      throw 'should not succeed';
    }).catch(function (err) {
      return true;
    });
  });
});
