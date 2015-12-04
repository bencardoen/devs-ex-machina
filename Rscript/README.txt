To generate the graphs included in the paper with your own benchmark results 
(after running the doBenchmark.py script):
	- put all the folders containing .csv files (aconnect, adevstone, aphold, 
		connect, devstone, ...) in a folder "data" (without the ")
	- run the R script

Note: by default the script will search for the data folder in the current
	working directory. To change this, you can edit the rootpath variable in the
	first lines of the script to your custom root path:
		rootpath <<- 'putyourcustomrootpathinhere'