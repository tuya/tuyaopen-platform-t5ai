#!/usr/bin/env python3

import os
import os.path

def main():
	if (os.path.exists(f'./properties/modules')):
		print('1')
	else:
		print('0')

if __name__ == "__main__":
	main()
