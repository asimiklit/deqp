# -*- coding: utf-8 -*-

import os
import re
import sys
import argparse
import subprocess

def readFile (filename):
	f = open(filename, 'rb')
	d = f.read()
	f.close()
	return d

def writeFile (filename, data):
	f = open(filename, 'wb')
	f.write(data)
	f.close()

def getHead (gitDir):
	out = subprocess.check_output(['git', '--git-dir', gitDir, 'rev-parse', 'HEAD'])
	return out.decode().strip()

def makeReleaseInfo (name, id):
	return """
/* WARNING: auto-generated file, use {genFileName} to modify */

#define DEQP_RELEASE_NAME	"{releaseName}"
#define DEQP_RELEASE_ID		{releaseId}
"""[1:].format(
		genFileName	= os.path.basename(__file__),
		releaseName	= name,
		releaseId	= id)

def parseArgs ():
	parser = argparse.ArgumentParser(description="Generate release info for build")
	parser.add_argument('--name', dest='releaseName', default=None, help="Release name")
	parser.add_argument('--id', dest='releaseId', default=None, help="Release ID (must be C integer literal)")
	parser.add_argument('--git', dest='git', action='store_true', default=False, help="Development build, use git HEAD to identify")
	parser.add_argument('--git-dir', dest='gitDir', default=None, help="Use specific git dir for extracting info")
	parser.add_argument('--out', dest='out', default=None, help="Output file")

	args = parser.parse_args()

	if (args.releaseName == None) != (args.releaseId == None):
		print "Both --name and --id must be specified"
		parser.print_help()
		sys.exit(-1)

	if (args.releaseName != None) == args.git:
		print "Either --name and --id, or --git must be specified"
		parser.print_help()
		sys.exit(-1)

	return args

if __name__ == "__main__":
	curDir			= os.path.dirname(__file__)
	defaultGitDir	= os.path.normpath(os.path.join(curDir, "..", "..", ".git"))
	defaultDstFile	= os.path.join(curDir, "qpReleaseInfo.inl")

	args = parseArgs()

	if args.git:
		gitDir		= args.gitDir if args.gitDir != None else defaultGitDir
		head		= getHead(gitDir)
		releaseName	= "git-%s" % head
		releaseId	= "0x%s" % head[0:8]
	else:
		releaseName	= args.releaseName
		releaseId	= args.releaseId

	releaseInfo	= makeReleaseInfo(releaseName, releaseId)
	dstFile		= args.out if args.out != None else defaultDstFile

	writeFile(dstFile, releaseInfo)
