#!/usr/bin/python2.7
import sys
import getopt
import os
import re

def usage():
    print "usage: smpirun.py --hostfile=hosts exec"

def read_hostfile(filedir, hosts):
    with open(filedir, 'r') as hostfile:
        for line in hostfile.readlines():
            line = line.strip('\n')
            tmp_list = re.split(' |=', line.lower())
            hostname = tmp_list[0]
            # The default value of slots is 1
            process_num = 1

            if 'slots' in tmp_list:
                id = tmp_list.index('slots')
                if len(tmp_list) > id + 1 and tmp_list[id + 1].isdigit():
                    process_num = tmp_list[id + 1]
            
            hosts.append((hostname, process_num))

def launch(hosts, exec_file, host_file):
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

    for host, process_num in hosts:
        #copy the hostfile to other nodes
        scp_cmd = 'scp {} {}:/tmp/'.format(host_file, host)
        os.system(scp_cmd)
        print scp_cmd
        for i in range(int(process_num)):
            ssh_cmd = 'ssh {} \'cd {}; {}\''.format(host, exec_absolute_path, exec_name)
            os.system(ssh_cmd)
            print ssh_cmd


hosts = []

opts, args = getopt.getopt(sys.argv[1:], "h", ["help", "hostfile=", "machinefile="])
hostfile = ''
for op, value in opts:
    if op == "-h" or op == "--help":
        usage()
        sys.exit()
    if op == "--hostfile" or op == "--machinefile":
        hostfile = value
        read_hostfile(hostfile, hosts)

if len(args) != 1:
    print "We need exactly one executable file."
    usage()
    sys.exit()

launch(hosts, args[0], hostfile)