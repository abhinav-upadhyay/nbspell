#!/usr/bin/env python

import argparse
import subprocess
import sys
import time

def parse_testdata(filepath):
    testdata = {}
    with open(filepath, 'r') as f:
        for line in f:
            line = line[:-1]
            words = line.split('\t')
            test = words[0]
            test = test.lower()
            answer = words[1]
            answer = answer.lower()
            if test not in testdata:
                testdata[test] = set()
            testdata[test].add(answer)
    return testdata


def get_corrections(tests, count):
    predicted_data = {}
    proc = subprocess.Popen(['./spell', '-c', count], stdin=subprocess.PIPE,
            stdout=subprocess.PIPE)
    out, err  = proc.communicate(input=tests)
    if proc.returncode != 0:
        print 'spell failed with error: %s' % str(err)
        print str(out)
        sys.exit(1)
    for line in out.split('\n'):
        if line == '':
            continue
        words = line.split()
        test = words[0]
        test = test[:-1]
        answers = words[-1].split(',')
        predicted_data[test] = answers
    return predicted_data    


def main():
    ncorrects = 0
    nwrongs = 0
    nfailed = 0
    argparser = argparse.ArgumentParser()
    argparser.add_argument('-i', '--input', help='path to the test data file', required=True)
    argparser.add_argument('-c', '--count', help='Number of suggestions to generate', default='1') 
    args = argparser.parse_args()
    starttime = time.time()
    testdata = parse_testdata(args.input)
    inputdata = '\n'.join(testdata.keys()) + '\n'
    predicted_data = get_corrections(inputdata, args.count)
    for test,answers in testdata.iteritems():
        corrections = predicted_data.get(test)
        if corrections is None or len(corrections) == 0:
            nfailed += 1
            print 'Failed to generate any prediction for %s' % test
            continue
        for correction in corrections:
            if correction in answers:
                ncorrects += 1
                break
        else:
            nwrongs += 1
            print 'Wrong prediction for %s, prediction: %s, expected: %s' % (test, correction, ','.join(answers))
    endtime = time.time()    

    print 'Tests finished in %f minutes' % ((endtime - starttime) / 60.0)
    print 'Total tests: %d' % len(testdata.keys())    
    print 'Total correct predictions: %d' % ncorrects
    print 'Total wrong predictions: %d' % nwrongs
    print 'Total tests where no correction was found: %d' % nfailed
    print 'Accuracy: %f' %  (ncorrects * 1.0 / (len(testdata.keys())))

if __name__ == '__main__':
    main()
