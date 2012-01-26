//FIXME
//var proteusDeviceInfo = require('../lib/proteusDeviceInfo.js');
var proteusDeviceInfo = require('proteusDeviceInfo');


var fs = require('fs');
var path = require('path');
var assert = require('assert');

var testNumber = 0;
var curTest ;
var curTestIndex = 0;


//-------------------------------------------Tests Loop up---------------------------------------------------------------------------------

function testInfo (func, shortName, type, level, description ,expectedResult) {

    this.shortName =  shortName;
    this.testType = type,
    this.autoLevel = level,
    this.func = func ,
    this.description = description ;
    this.result = "Not Run";
    this.expectedResult = expectedResult;

    function setResult(res)
    {
      this.result = res;
    }

}

var testList = [
		new testInfo(testGetSysPropValid , 'Test_Get_SysProp_Valid', 'L1', 'AUTO',"This is to test passing a valid system property",""),
		new testInfo(testGetSysPropInvalid , 'Test_Get_SysProp_InValid', 'L1', 'AUTO',"This is to test passing a invalid system property",""),
		new testInfo(testGetDeviceInfo, 'Test_Get_Device_Info', 'L1', 'AUTO',"This is to test passing a invalid system property",""),
		new testInfo(testGetEnvPropValid , 'Test_Get_EnvProp_Valid', 'L1', 'AUTO',"This is to test passing a valid environment property",""),
		new testInfo(testGetEnvPropInValid , 'Test_Get_EnvProp_InValid', 'L1', 'AUTO',"This is to test passing a invalid environment property","")

	       ];

//-------------------------------------------Tests Loop up---------------------------------------------------------------------------------


//-------------------------------------------getResults--------------------------------------------------------------------------------

function getResults(testList){
  var result = '<table border="1">' ;
  result += '<tr> <th>Name</th> <th>Test Level</th>  <th>Automation Level</th> <th>Description</th><th>Expected Result</th><th>Result</th></tr> ';

  for (i in testList){
      result+= '<tr> <td>' +  testList[i].shortName + '</td> <td>' + testList[i].testType + '</td> <td>' + testList[i].autoLevel + '</td> <td>' + testList[i].description + '</td> <td>' +  testList[i].expectedResult + '</td> <td>'  +  testList[i].result  + '</td> </tr>' ;


      console.log("Result  : " + JSON.stringify(testList[i]));
    }

  result += '</table>';

  // write result to a HTML file
  writeTestResult(result);
  return result;
}

function writeTestResult(result){
  var html = '<html> <body> <p1> Unzip Module Test Results </p1>' + result +'</body></html>';
  var filePath = process.downloadPath + "/DeviceInfo_Module_Test_Results.html";

  if (path.existsSync(filePath))
    fs.unlinkSync(filePath);
  var err = fs.writeFileSync(filePath, html);
  if(err) {
    console.log("Error Writing Results " +err);
    return ;
  }
  else {
    console.log("The Results file was saved!");
  }
}
//-------------------------------------------getResults--------------------------------------------------------------------------------

//-------------------------------------------Tests Util--------------------------------------------------------------------------------


//-------------------------------------------Tests Util--------------------------------------------------------------------------------

//-------------------------------------------Tests Executer--------------------------------------------------------------------------------



exports.deviceInfoTests = function (callback){
  run();

function run (){
  console.log("Running Tests "+ curTestIndex +"/" +testList.length);
  curTest = testList[curTestIndex++];
  if(!curTest){
    console.log('All Tests have been completed');
    var result = getResults(testList);
    callback(result);
  }
  else{
    curTest.func(run);
  }
 }

}
//-------------------------------------------Tests Executer--------------------------------------------------------------------------------


//-------------------------------------------Tests --------------------------------------------------------------------------------


function testGetSysPropValid(callback){

  console.log('testGetPropValid start');

  var propVal = proteusDeviceInfo.getSystemProperty("ro.product.model");
  console.log('testGetPropValid <ro.product.model> --- >  : < '+ propVal + ' >');
  if (propVal){
    console.log('testGetPropValid test : ' + 'PASS');
    curTest.result = "PASS";
  }
  else{
    console.log('testGetPropValid test : ' + 'FAIL');
    curTest.result = "FAIL";
  }

  //var propVal = proteusDeviceInfo.getSystemProperty("Pictures");
  //console.log('testGetPropValid <Pictures> --- >  : < '+ propVal + ' >');

  callback();

}//End of testGetPropValid


function testGetEnvPropValid(callback){
  console.log('testGetEnvPropValid start');

  var propVal = proteusDeviceInfo.getEnvironmentProperty("DIRECTORY_MUSIC");
  console.log('testGetEnvPropValid <DIRECTORY_MUSIC> --- >  : < '+ propVal + ' >');
  if (propVal){
    console.log('testGetEnvPropValid test : ' + 'PASS');
    curTest.result = "PASS";
  }
  else{
    console.log('testGetEnvPropValid test : ' + 'FAIL');
    curTest.result = "FAIL";
  }
  callback();
}// End of testGetEnvPropValid

function testGetEnvPropInValid(callback){
  console.log('testGetEnvPropInvalid start');
  try{
  var propVal = proteusDeviceInfo.getEnvironmentProperty("MusicInvalid");
  console.log('testGetEnvPropInvalid <Music> --- >  : < '+ propVal + ' >');
  console.log('testGetEnvPropInvalid test : ' + 'FAIL');
  curTest.result = "FAIL";
  }
  catch(ex){
    console.log('testGetEnvPropInvalid test : ' + ex);
    console.log('testGetEnvPropInvalid test : ' + 'PASS');
    curTest.result = "PASS";
  }
  callback();
}// End of testGetEnvPropInvalid

function testGetSysPropInvalid(callback){

  console.log('testGetPropInvalid start');

  var propVal = proteusDeviceInfo.getSystemProperty("ro.pduct.model");
  console.log('testGetPropInvalid <ro.product.model> --- >  : < '+ propVal + ' >');
  if (propVal){
    console.log('testGetPropInvalid test : ' + 'FAIL');
    curTest.result = "FAIL";
  }
  else{
    console.log('testGetPropInvalid test : ' + 'PASS');
    curTest.result = "PASS";
  }


  callback();

}//End of testGetPropInvalid


function testGetDeviceInfo(callback){

  console.log('testGetDeviceInfo start');

  var deviceInfo = proteusDeviceInfo.getDeviceInfo();
  console.log('testGetDeviceInfo <deviceInfo> --- >  : < '+ deviceInfo + ' >');
  if (deviceInfo){
    console.log('testGetDeviceInfo test : ' + 'PASS');
    curTest.result = "PASS";
  }
  else{
    console.log('testGetDeviceInfo test : ' + 'FAIL');
    curTest.result = "FAIL";
  }


  callback();

}//End of testGetDeviceInfoInvalid
//
