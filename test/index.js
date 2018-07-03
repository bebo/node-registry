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

  it('get key binary', function () {
    return Registry.getValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Elgato Systems GmbH\\StreamDeck",
      key: "Devices",
    }).then(function (data) {
      if (!(data.value instanceof Buffer)) {
        throw "Value is not buffer type";
      }
      return true;
    }).catch(function (err) {
      throw err;
    });
  });


  it('put key', function () {
    return Registry.putValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\Test",
      type: "REG_QWORD",
      value: "9223372036854775800",
      key: "TestPutKey",
    }).then(function (data) {
      return true;
    }).catch(function (err) {
      throw err;
    });
  });

  it('delete key', function () {
    return Registry.putValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\Test",
      type: "REG_SZ",
      value: "test",
      key: "TestDeleteKey",
    }).then(function (data) {
      return Registry.deleteValue({
        hkey: "HKEY_CURRENT_USER",
        subkey: "SOFTWARE\\Bebo\\Test",
        key: "TestDeleteKey",
      })
    }).then(function (data) {
      return true;
    }).catch(function (err) {
      throw err;
    });
  });

  it('create key - not create if exists', function () {
    return Registry.putValue({ // force to write
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\Test",
      type: "REG_SZ",
      key: "TestCreateKey",
      value: "test - create (put)",
    }).then(function (data) {
      return Registry.createValue({
        hkey: "HKEY_CURRENT_USER",
        subkey: "SOFTWARE\\Bebo\\Test",
        type: "REG_SZ",
        key: "TestCreateKey",
        value: "test - create (create) should not happen",
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

  it('unsigned - dword', () => {
    const hkey = "HKEY_CURRENT_USER";
    const subkey = "SOFTWARE\\Bebo\\Test";
    const key = "Test_DWORD_unsigned_1";

    return Registry.putValue({
      hkey,
      subkey,
      key,
      type: "REG_DWORD",
      value: "4294967295"
    }).then(function (data) {
      return Registry.getValue({
        hkey,
        subkey,
        key
      });
    }).then(function (data) {
      if (data.value != 0xffffffff) {
        throw "data.value != 0xffffffff";
      }
      return true;
    }).catch(function (err) {
      throw err;
    });
  });

  it('unsigned - dword', () => {
    const hkey = "HKEY_CURRENT_USER";
    const subkey = "SOFTWARE\\Bebo\\Test";
    const key = "Test_DWORD_unsigned_2";
    return Registry.putValue({
      hkey,
      subkey,
      key,
      type: 'REG_DWORD',
      value: "-1"
    }).then(function (data) {
      return Registry.getValue({
        hkey,
        subkey,
        key
      });
    }).then(function (data) {
      if (data.value != 0xffffffff) {
        throw "data.value != 0xffffffff";
      }
      return true;
    }).catch(function (err) {
      throw err;
    });
  });


  it('invalid argument qword', () => {
    return Registry.putValue({
      hkey: "HKEY_CURRENT_USER",
      subkey: "SOFTWARE\\Bebo\\Test",
      type: "REG_QWORD",
      key: "Test_QWORD_invalid",
      value: "test",
    }).then(function (data) {
      throw 'should not succeed';
    }).catch(function (err) {
      if (err != 'value - invalid argument') throw err;
      return true;
    });
  });


});
