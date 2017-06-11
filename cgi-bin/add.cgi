#!/usr/bin/python

import cgi
import cgitb
cgitb.enable()

import os
import string

print 'Content-Type:text/html'
print
query_string = os.environ['QUERY_STRING']
args = string.split(query_string, '&')
key_val_list = [string.split(s, '=') for s in args]
arg_dict = {s[0]: s[1] for s in key_val_list}

print '<h1>Results</h1>'
num1 = int(arg_dict['num1'])
num2 = int(arg_dict['num2'])
sum = num1 + num2
print '<p>{0} + {1} = {2}</p>'.format(num1, num2, sum)
