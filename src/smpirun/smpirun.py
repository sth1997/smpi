#!/usr/bin/python2.7
import sys
import getopt
import os
import re
from multiprocessing import Process

def usage():
    print "usage: smpirun.py --hostfile=hosts [--use-smartnic] exec"

def read_hostfile(filedir, hosts):
    with open(filedir, 'r') as hostfile:
        for line in hostfile.readlines():
            line = line.strip('\n')
            tmp_list = re.split(' |=', line.lower())
            hostname = tmp_list[0]
            # The default value of slots is 1
            process_num = 1
            smartnic = ""

            if 'slots' in tmp_list:
                id = tmp_list.index('slots')
                if len(tmp_list) > id + 1 and tmp_list[id + 1].isdigit():
                    process_num = int(tmp_list[id + 1])
            
            if 'smartnic' in tmp_list:
                id = tmp_list.index('smartnic')
                if len(tmp_list) > id + 1:
                    smartnic = tmp_list[id + 1]
            
            if process_num != 1:
                print "Now, only supports slots=1"
                sys.exit()
            
            hosts.append((hostname, process_num, smartnic))

def checkSmartnic(hosts):
    for tup in hosts:
        if tup[2] == "":
            print "If use smartnic, there must be one smartnic set for each host in hostfile."
            sys.exit()


def launch_thread(ssh_cmd):
    print ssh_cmd
    os.system(ssh_cmd)

def launch(hosts, exec_file, host_file, otherArgs, useSmartnic):
    pwd = os.getcwd()
    tmp_list = exec_file.split('/')
    exec_name = tmp_list[-1]
    if exec_file[0] == '/':
        exec_absolute_path = '/'.join(tmp_list[ : -1])
    else:
        exec_absolute_path = pwd + '/' + '/'.join(tmp_list[ : -1])
    if len(tmp_list) != 1:
        exec_absolute_path = exec_absolute_path + '/'
    print "exec = " + exec_absolute_path + exec_name
    
    size = 0
    for host, process_num, smartnic in hosts:
        size = size + process_num

    rank = 0
    for host, process_num, smartnic in hosts:
        #copy the hostfile to all hosts
        scp_cmd = 'scp {} {}:/tmp/hosts'.format(host_file, host)
        os.system(scp_cmd)
        useSmartnicArg = ""
        if useSmartnic:
            useSmartnicArg = "--use-smartnic"
        for i in range(process_num):
            ssh_cmd = 'ssh {} \'cd {}; if [ -f {} ];then \
                        ./{} --smpirank {} --smpisize {} {} {}; \
                        else \
                        {} --smpirank {} --smpisize {} {} {}; \
                        fi\''.format(host, exec_absolute_path, exec_name, exec_name, str(rank), str(size), useSmartnicArg, otherArgs, exec_name, str(rank), str(size), useSmartnicArg, otherArgs)
            t = Process(target=launch_thread, args=(ssh_cmd,))
            t.start()
            rank = rank + 1
    
    if useSmartnic:
        rank = 0
        for host, process_num, smartnic in hosts:
            #copy the hostfile to all smartnics
            scp_cmd = 'scp {} {}:/tmp/hosts'.format(host_file, smartnic)
            os.system(scp_cmd)
            for i in range(process_num):
                """ssh_cmd = 'ssh {} \'cd {}; if [ -f {} ];then \
                            ./{} --smpirank {} --smpisize {} --is-smartnic {}; \
                            else \
                            {} --smpirank {} --smpisize {} --is-smartnic {}; \
                            fi\''.format(smartnic, exec_absolute_path, exec_name, exec_name, str(rank), str(size), otherArgs, exec_name, str(rank), str(size), otherArgs)"""
                ssh_cmd = 'ssh {} \'run_smartnic --smpirank {} --smpisize {} --is-smartnic {};\''.format(smartnic, str(rank), str(size), otherArgs)
                t = Process(target=launch_thread, args=(ssh_cmd,))
                t.start()
                rank = rank + 1
            


hosts = []
useSmartnic = False

opts, args = getopt.getopt(sys.argv[1:], "hs", ["help", "hostfile=", "machinefile=", "use-smartnic"])
hostfile = ''
for op, value in opts:
    if op == "-h" or op == "--help":
        usage()
        sys.exit()
    if op == "--hostfile" or op == "--machinefile":
        hostfile = value
        read_hostfile(hostfile, hosts)
    if op == "-s" or op == "--use-smartnic":
        useSmartnic = True

"""
if len(args) != 1:
    print "We need exactly one executable file."
    usage()
    sys.exit()
"""
if hosts == [] or len(args) == 0:
    usage()
    sys.exit()

if useSmartnic:
    checkSmartnic(hosts)

if len(args) > 1:
    otherArgs = ' '.join(args[1:])
else:
    otherArgs = ''
launch(hosts, args[0], hostfile, otherArgs, useSmartnic)
