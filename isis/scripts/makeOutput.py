'''
Script to create truthData for unit tests.

For output it expects the command in the form of:
    python makeOutput.py testName

where testname is the cmake name for the unit or app test

To check in truth data the command should be in the form of:
    python makeOutput.py testName truth

The unit tests stdout and stderr is redirected to the file
named object.truth where object is the ISIS object being tested.

'''

import sys
import os


if not os.environ['ISISROOT']:
    print("The $ISISROOT variable is not set")
    sys.exit()

builddir = os.environ['ISISROOT']

if "_unit_" in sys.argv[1]:
    unitTestExecutable = sys.argv[1]

    unitTestName = unitTestExecutable.split("_test_")[1] + ".truth"
    unitTestPath = builddir + "/unitTest"

    os.system(unitTestPath + "/" + unitTestExecutable + ">&" + unitTestPath + "/" + unitTestName)
    print("Unit Test Output In " + unitTestPath + " As " + unitTestName)

    if len(sys.argv) >= 3 and sys.argv[2] == "truth":
        with open(builddir + "/objects/CTestTestfile.cmake") as testFile:
            for line in testFile:
                if unitTestName in line:
                    unitTestSrcPath = line.split("\" \"")[2][13:]
                    os.system("cp -f " + unitTestPath + "/" + unitTestName + " " + unitTestSrcPath)
                    break

            print("Checked In Truth Data To " + unitTestSrcPath)
