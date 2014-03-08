import os
import re
import stat
import subprocess

mapping = {}

re_line = re.compile('(.+) (.+)')

def fix_includes(outputpath, inputpath, srcpath):
   input = open(inputpath, 'r')
   lines = input.readlines()
   input.close()

   output = reallyopen(outputpath, 'w')

   filename = os.path.basename(mapping[srcpath])
   filedir = os.path.dirname(srcpath)+'/'
   re_include = re.compile('^( *[#.])include "(.*)"(.*)\n$')
   re_ifndef = re.compile('^#ifndef ([^_]\w+_H)\n$')
   re_define = re.compile('^#define ([^_]\w+_H)\n$')
   last_ifndef = None
   for line in lines:
      match = re_include.match(line)
      if match:
         indent = match.group(1)
         path = match.group(2)
         comment = match.group(3)
         oldpath = path
         if os.path.normpath(filedir + path).replace('\\','/') in mapping:
            # Turn relative path into absolute.
            path = filedir + path
         path = os.path.normpath(path)
         npath = path
         path = path.replace('\\','/')
         rpath = path
         if path in mapping:
            path = mapping[path]
         if path not in ['rpc_platform.h','khronos_platform_types.h','khrn_platform_interlock.h','vg_platform_config.h','vg_platform_scissor.h','vg_platform_path.h','vg_platform_ramp.h'] and ('/' not in path or path.find('..') != -1):
            print "NO " + os.path.normpath(filedir + path).replace('\\','/')
            print "["+filedir+"]"+oldpath + " -> " + npath + " -> " + rpath + " -> " + path
            assert(0)
         line = indent + 'include "' + path + '"' + comment + '\n'

      if last_ifndef != None:
         match = re_define.match(line)
         if match:
            token = filename.upper().replace('.','_')
            line = "#ifndef "+token+"\n#define "+token+"\n"
         else:
            line = "#ifndef "+last_ifndef+"\n"+line
         last_ifndef = None

      match = re_ifndef.match(line)
      if match:
         last_ifndef = match.group(1)
         line = ''

      output.write(line)
   assert(last_ifndef == None)

def read_mapping(file):
   result = {}
   for line in file:
      match = re_line.match(line)
      assert(match)
      name0 = match.group(1)
      name1 = match.group(2)
      if name0 in result or name1 in result.values():
         print name0
         assert(0)
      result[name0] = name1
   return result

def p4_move():
   subprocess.call(['p4','edit','middleware/khronos/...'])
   for src in mapping:
      dst = mapping[src]
      subprocess.call(['p4','move',src,dst])

def new_file_change_include():
   for src in mapping:
      dst = mapping[src]
      print dst
      assert(dst != src)
      fix_includes(dst, src, src)

def in_place_change_include():
   for src in mapping:
      dst = mapping[src]
      print dst
      fix_includes(dst, dst, src)

def reallyopen(path, mode):
   dir = os.path.dirname(path)
   if not os.path.exists(dir):
      os.makedirs(dir)
   return open(path, mode)

def makefile(sfile,dfile,cfile):
   sdirs = []
   snames = []
   cdirs = []
   cnames = []
   for src in mapping:
      dst = mapping[src]
      (blah, ext) = os.path.splitext(dst)
      if ((dst.find('2707')==-1 and dst.find('_cr')==-1 and dst.find('linux')==-1 and dst.find('win32')==-1 and dst.find('vcos')==-1 and dst.find('selfhosted')==-1 and dst.find('direct/')==-1 and dst.find('wf')==-1) and dst.find('dispatch')==-1 and dst.find('khrn_misc')==-1 and dst.find('egl_brcm_global_image_id')==-1 and dst.find('khrn_int_generic_map.c')==-1 and dst.find('khrn_visual_stats')==-1 or dst in ['middleware/khronos/glxx/glxx_server_cr.c','middleware/khronos/glsl/2707b/glsl_fpu3.c']) and ext not in ['.h','.inc','.qinc','.qasm','.l','.y','.mk','.bat','.arm']:
         dir = os.path.dirname(dst) + '/'
         name = os.path.basename(dst)
         assert(name not in snames)
         assert(name not in cnames)
         if dir.startswith('middleware/khronos/'):
            dir = dir[19:]
            if dir not in sdirs:
               sdirs.append(dir)
            snames.append(name)
         elif dir.startswith('interface/khronos/'):
            dir = dir[18:]
            if dir not in cdirs:
               cdirs.append(dir)
            cnames.append(name)
         else:
            print dir
            assert(0)
   sdirs.sort()
   snames.sort()
   cdirs.sort()
   cnames.sort()
      
   sfile.write('LIB_NAME := khronos_main\nLIB_SRC :=\nLIB_VPATH :=\n')
   sfile.write('LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)\nLIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)\nDEFINES_GLOBAL += EGL_SERVER_SMALLINT\n')
   sfile.write('LIB_VPATH := ')
   for dir in sdirs:
      if dir != '':
         sfile.write(dir[:-1] + ' ')
   sfile.write('\nLIB_SRC := ')
   for name in snames:
      sfile.write(name + ' ')
   sfile.write('\nLIB_IPATH := \n')
      
   dfile.write('LIB_NAME := khronos_direct\nLIB_SRC :=\nLIB_VPATH :=\n')
   dfile.write('LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)\nLIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)\nDEFINES_GLOBAL += EGL_SERVER_SMALLINT\nLIB_DEFINES := RPC_DIRECT\n')
   dfile.write('LIB_LIBS := middleware/khronos/khronos_main\n')
   dfile.write('LIB_VPATH := ../../interface/khronos/platform/direct common ')
   for dir in cdirs:
      dfile.write(('../../interface/khronos/' + dir)[:-1] + ' ')
   dfile.write('\nLIB_SRC := khrn_client_direct.c khrn_misc.c ')
   for name in cnames:
      dfile.write(name + ' ')
   dfile.write('\nLIB_IPATH := \n')
      
   cfile.write('LIB_NAME := khronos_client\nLIB_SRC :=\nLIB_VPATH :=\n')
   cfile.write('LIB_CFLAGS := $(WARNINGS_ARE_ERRORS)\nLIB_AFLAGS := $(ASM_WARNINGS_ARE_ERRORS)\n')
   cfile.write('LIB_VPATH := ../../../../interface/khronos/platform/selfhosted ../../../../interface/khronos/platform/vcos ../../interface/khronos/common ../../interface/khronos/egl ../../interface/khronos/ext ../../interface/khronos/glxx ../../interface/khronos/vg ')
   for dir in cdirs:
      cfile.write(('../../../../interface/khronos/' + dir)[:-1] + ' ')
   cfile.write('\nLIB_SRC := khrn_client_selfhosted.c khrn_client_rpc_selfhosted.c khrn_client_vcos.c ')
   for name in cnames:
      cfile.write(name + ' ')
   cfile.write('\nLIB_IPATH := \n')

def full_listing(path):
   result = []
   for filename in os.listdir(path):
      fullname = path + filename
      mode = os.stat(fullname)[stat.ST_MODE]
      if (stat.S_ISDIR(mode)):
         full_listing(fullname + '/')
      else:
         result.append(path + filename)
   result.sort()
   return result

def show_missing_files():
   files = full_listing('middleware/khronos/')
   for f in files:
      if f not in mapping:
         print "MISSING " + f

def branch_spec(srcpath, dstpath, m2):
   for src in mapping:
      dst = dstpath + mapping[src]
      if m2 and 'xxx/'+src in m2:
         src = m2['xxx/'+src]
      else:
         src = srcpath + src
      print src + ' ' + dst

os.chdir('../../../../')
mapping = read_mapping(open('middleware/khronos/tools/reorg/mapping.txt','r'))
#new_file_change_include()
#show_missing_files()

#makefile(open('middleware/khronos/khronos_main.mk','w'),open('middleware/khronos/khronos_direct.mk','w'),open('middleware/khronos/client/vcfw/khronos_client.mk','w'))
#p4_move()
#in_place_change_include()
hauxwell_mapping = read_mapping(open('middleware/khronos/tools/reorg/hauxwell_mapping.txt','r'))
branch_spec('//software/projects/hauxwell/vc4_to_ansi/','//software/vc4/DEV/', hauxwell_mapping)

