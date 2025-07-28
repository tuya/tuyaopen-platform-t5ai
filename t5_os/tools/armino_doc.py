#!/usr/bin/env python3


import os
import subprocess
import sys
import argparse
import glob

PRINT_READ = "\033[91m"
PRINT_RESET = "\033[0m"

def run_cmd(cmd):
	process = subprocess.Popen(cmd, shell=True)
	process.wait()

	return process

def log_error(log):
	print(PRINT_READ + log + PRINT_RESET)

def print_error_lines(file_path, error_log):
	ret = False

	try:
		with open(file_path, 'r') as file:
			for line in file:
				if error_log in line:
					if ret is False:
						log_error(f"Error found in file: {file}")
						ret = True

					log_error(line.strip())

	except FileNotFoundError:
		log_error("File not found!")
		ret = True
	except Exception as e:
		log_error("An error occurred: " + str(e))
		ret = True

	return ret


def latex_error_check(path):
	#print("check path: " + path)
	ret = False

	files = glob.glob(f"{path}/*.log")

	for file in files:
		ret = print_error_lines(file, "Error:")

	return ret


def build_armino_doc(source_path, dest_path, build_path, landir, version):
	print("found souce: " + source_path + " dest: " + dest_path + " build: " + build_path)
	command = "make -C " + source_path + " arminodocs -j32 " + "TARGET_DIR=" + dest_path + " TARGET_VERSION=" + version
	print("\t" + command)


	#clean before build
	run_cmd(f'rm -rf {build_path}/{landir}')
	run_cmd(f'rm -rf {source_path}/source')

	run_cmd(command)

	if latex_error_check(f'{source_path}/_build/latex') is True:
		log_error("### Build Docs Error, Exit ###")
		exit(-1)

	#copy
	run_cmd(f'cp -rf {dest_path} {build_path}/{landir}')
  
	#clean after build
	run_cmd(f'rm -rf {source_path}/xml')
	run_cmd(f'rm -rf {source_path}/xml_in')
	run_cmd(f'rm -rf {source_path}/man')
	run_cmd(f'rm -rf {source_path}/../__pycache__')
	run_cmd(f'rm -rf {dest_path}')
	run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/inc')
	run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/latex')

def build_html(source_path, dest_path, build_path, landir):
	print("found souce: " + source_path + " dest: " + dest_path + " build: " + build_path)
	command = "make -C " + source_path + " arminodocs -j32 " + "TARGET_DIR=" + dest_path
	print("\t" + command)


	#clean before build
	run_cmd(f'rm -rf {build_path}/{landir}')

	if run_cmd(command) is not True:
		log_error("### Build Docs Error, Exit ###")
		exit(-1)

	#copy
	run_cmd(f'cp -rf {dest_path} {build_path}/{landir}')
	#run_cmd(f'cp -rf {build_path}/{landir}/latex/AVDKDocument.pdf {build_path}/{landir}/AVDKDocument.pdf')
  
	#clean after build
	#run_cmd(f'rm -rf {source_path}/xml')
	#run_cmd(f'rm -rf {source_path}/xml_in')
	#run_cmd(f'rm -rf {source_path}/man')
	#run_cmd(f'rm -rf {docs_path}/__pycache__')
	#run_cmd(f'rm -rf {dest_path}')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/inc')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/latex')

def build_pdf(source_path, dest_path, build_path, landir):
	print("found souce: " + source_path + " dest: " + dest_path + " build: " + build_path)
	command = "make -C " + source_path + " latexpdf " + "TARGET_DIR=" + dest_path
	print("\t" + command)


	#clean before build
	run_cmd(f'rm -rf {build_path}/{landir}')

	run_cmd(command)

	#copy
	run_cmd(f'cp -rf {dest_path} {build_path}/{landir}')
	#run_cmd(f'cp -rf {build_path}/{landir}/latex/AVDKDocument.pdf {build_path}/{landir}/AVDKDocument.pdf')
  
	#clean after build
	#run_cmd(f'rm -rf {source_path}/xml')
	#run_cmd(f'rm -rf {source_path}/xml_in')
	#run_cmd(f'rm -rf {source_path}/man')
	#run_cmd(f'rm -rf {docs_path}/__pycache__')
	#run_cmd(f'rm -rf {dest_path}')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/inc')
	#run_cmd(f'rm -rf {dest_path} {build_path}/{landir}/latex')

def build_doc(target, docs_path, build_path, version):
	print("build %s docs" % target)

	subdirectories = [d for d in os.listdir(docs_path) if os.path.isdir(os.path.join(docs_path, d))]
	white_list = {"en", "zh_CN"}
	target_dirs = [x for x in subdirectories if x in white_list]

	if not os.path.exists(build_path):
		run_cmd(f'mkdir -p {build_path}')

	for subdir in target_dirs:
		source_path = docs_path + "/" + subdir
		#dest_path = build_path + "/" + target + "/" + subdir
		dest_path = docs_path + "/" + subdir + "/_build"
		#build_html(source_path, dest_path, build_path, subdir)
		#build_pdf(source_path, dest_path, build_path, subdir)
		build_armino_doc(source_path, dest_path, build_path, subdir, version)

def build_all(docs_path, build_path, version):
	print("build all docs")

	subdirectories = [d for d in os.listdir(docs_path) if os.path.isdir(os.path.join(docs_path, d))]
	black_list = {"common", ".git"}
	target_dirs = [x for x in subdirectories if x not in black_list]

	for subdir in target_dirs:
		print("found target: " + subdir)
		build_doc(subdir, docs_path + "/" + subdir, build_path + "/" + subdir, version)

def main(argv):
	parser = argparse.ArgumentParser()
	parser.add_argument('--clean', type=bool, default=False)
	parser.add_argument('--target', type=str, default="all")
	parser.add_argument('--type', type=str, default="html")
	parser.add_argument('--version', type=str, default="latest")
	args = parser.parse_args()

	root_path = os.getcwd()
	if 'ARMINO_SOC' in os.environ:
		soc_name = os.getenv('ARMINO_SOC')
	else:
		raise RuntimeError("not get soc name")
	build_path = root_path + f"/build/doc/smp_doc"
	if os.path.exists(build_path) == False:
		os.makedirs(build_path, exist_ok=True)

	if args.clean and args.target == "all":
		return

	run_cmd(f'cp {root_path}/docs/version.json {build_path}/version.json')

	if args.clean == False and args.target == "all":
		build_all(root_path + "/docs", build_path, args.version)
		return
 
	if os.path.exists(root_path + "/docs/" + args.target):
		build_doc(args.target, root_path + "/docs/" + args.target, build_path, args.version)
	else:
		print("Error not found target: %s" % (root_path + "/docs/" + args.target))

if __name__ == "__main__":
	main(sys.argv)
