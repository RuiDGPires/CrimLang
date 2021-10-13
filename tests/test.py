#!/usr/bin/env python3
# SCRIPT TO TEST THE PROJECT

from glob import glob
import subprocess
import tempfile 
import os
import argparse
import sys
import datetime
from pathlib import Path

HEADER = '\033[95m'
OKBLUE = '\033[94m'
OKCYAN = '\033[96m'
OKGREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
ENDC = '\033[0m'
BOLD = '\033[1m'
UNDERLINE = '\033[4m'

dir_path = dir_path = os.path.dirname(os.path.realpath(__file__))

EXE = f"{dir_path}/../crimlang_dbg"
VM = f"{dir_path}/../../CrimVm/crimvm"

TMP = f"{dir_path}/tmp.cas"

DIRS = next(os.walk(f"{dir_path}/inputs/."))[1]



def printError(string):
    print(FAIL + BOLD + string + ENDC)

def printSucess(string):
    print(OKGREEN + BOLD + string + ENDC)

def printProgressBar(n, progress):
    percent = round((progress/n) * 100)
    BAR_SIZE = 20
    x = (progress * BAR_SIZE) // n
    color = ""

    if percent < 50:
        color = FAIL
    elif percent != 100:
        color = WARNING
    else:
        color = OKGREEN

    text = "[" + color
    for i in range(x):
        text += "#"
    for i in range(BAR_SIZE - x):
        text += "-"

    text += ENDC + f"] ({color}{percent}%{ENDC})"
    print(text)

n_files = 0
n_files_passed = 0

print()
ALL_TESTS_PASSED = True
for directory in DIRS:
    if not os.path.exists(f"{dir_path}/inputs/{directory}/.section"): continue

    section_tests_passed = True
    INPUT_FILES = sorted(glob(f"{dir_path}/inputs/{directory}/*"))
    t = str.maketrans("\n", " ") 

    section_name = ""
    with open(f"{dir_path}/inputs/{directory}/.section") as f:
        section_name = f.readlines()[0].translate(t) 

    if section_name[-1] == " ":
        section_name = section_name[:-1]
    
    section_text = ""

    for file in INPUT_FILES:
        if os.path.splitext(file)[-1] != ".crim": continue
        # Check if test file needs input file
        flg_needs_input = False
        flg_test_error = False
        flg_test_return = False

        test_flags = ""
        with open(file, 'r') as f:
            test_flags = f.readline().replace(" ", "").replace("\n", "")
        
        if test_flags[0:4] == "//@[":
            assert test_flags[-1] == ']'
            test_flags = test_flags[4:-1].split(",")
            for flag in test_flags:
                if flag.upper() == "IN":
                    flg_needs_input = True
                elif flag.upper() == "ERR":
                    flg_test_error = True
                elif flag.upper() == "RET":
                    flg_test_return = True

        # GET FILE NAME
        filename = os.path.basename(file)

        n_files += 1
        filename_no_ext = os.path.splitext(filename)[0] 
        out_file = f"{dir_path}/outputs/{directory}/{filename_no_ext}.out"

        # EXECUTE PROGRAM
        result = 0
        


        a = subprocess.run([EXE, file, TMP], stdout = subprocess.PIPE, stderr = subprocess.PIPE)

        if not flg_needs_input:
            result = subprocess.run([VM, "-ar", TMP], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        else:
            with open(f"{dir_path}/inputs/{directory}/{filename_no_ext}.in", "r") as test_input:
                result = subprocess.run([VM, "-ar", TMP], stdin = test_input, stdout = subprocess.PIPE, stderr = subprocess.PIPE)

        ret_code = result.returncode 

        success = False
        if not flg_test_return:
            success = ret_code == 0 
        else:
            success = True

        passed = False 

        if success:
            with open(out_file, "r") as f:
                file_text = f.read();
            if flg_test_return:
                passed = file_text == str(ret_code) + "\n"
            else:
                passed = file_text == result.stdout.decode('ascii')
        else:
            if flg_test_error:
                with open(out_file, "r") as f:
                    file_text = f.read();
                passed = file_text == result.stderr.decode('ascii')
            

        if not passed:
            section_text += FAIL + "fail " + ENDC + ":\t" + filename + "\n"
            ALL_TESTS_PASSED = False
            section_tests_passed = False
        else:
            section_text += OKGREEN + "ok   " + ENDC + ":\t" + filename + "\n" 
            n_files_passed += 1 
   
    sec_color = ""
    if section_tests_passed:
        sec_color = OKGREEN
    else:
        sec_color = FAIL

    print(WARNING, BOLD, f"  SECTION [", ENDC + sec_color, section_name, ENDC + WARNING + BOLD, "]\n", ENDC)
    print(section_text)

print("\n-------\n")
printProgressBar(n_files, n_files_passed)

if ALL_TESTS_PASSED:
    printSucess("ALL TESTS PASSED!\n")

os.remove(TMP)
