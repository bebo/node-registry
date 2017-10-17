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
      return true;
    }).catch(function (err) {
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
      return true;
    }).catch(function (err) {
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
      return true;
    }).catch(function (err) {
      throw err;
    });
  });

  it('create key', function () {
    return Registry.putValue({ // force to write
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\GameCapture",
      type: "REG_SZ",
      key: "TestCreateKey",
      value: "test",
    }).then(function (data) {
      return Registry.createValue({
        hkey: "HKEY_CURRENT_USER",
        subkey: "SOFTWARE\\Bebo\\GameCapture",
        type: "REG_SZ",
        key: "TestCreateKey",
        value: "test - fail",
      })
    }).then((data) => {
      throw 'should not succeed';
    }).catch(function (err) {
      if (err != 'Error: Key already exists') throw err;
      return true;
    })
  });

  it('invalid argument qword', () => {
    return Registry.putValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\Test",
      type: "REG_QWORD",
      key: "Testing122232",
      value: "test",
    }).then(function (data) {
      throw 'should not succeed';
    }).catch(function (err) {
      if (err != 'value - invalid argument') throw err;
      return true;
    });
  });

  it('invalid argument dword', () => {
    return Registry.putValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\Test",
      type: "REG_DWORD",
      value: "test",
      key: "TestingDWORD",
    }).then(function (data) {
      throw 'should not succeed';
    }).catch(function (err) {
      if (err != 'value - invalid argument') throw err;
      console.error(err);
      return true;
    });
  });
});
