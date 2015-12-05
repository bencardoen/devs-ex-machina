# global variables
generatePDF <<- FALSE	# if TRUE, generate PDF else: generated EPS files...
epscount <<- 1
rootpath <<- paste(getwd(), '/', sep = '')
# rootpath <<- 'putyourcustomrootpathinhere'
datapath <<- paste(rootpath, 'data/', sep = '')
figspath <<- paste(rootpath, 'figs/', sep = '')
dir.create(figspath, showWarnings = FALSE)
setwd(datapath)

# 1-dimensional performance tests

process1ddata <- function(datatable, collabel) {
	print(sprintf("process1ddata: %s", collabel))
	resultdata <- data.frame(x = numeric(), cpu = numeric(), conflow = numeric(), confhigh = numeric())
		
	timelabel = 'time.elapsed..seconds.'

	colindex = which(colnames(datatable) == collabel)
	timeindex = which(colnames(datatable) == timelabel)
	print(sprintf("Column Index: %s %d", collabel, colindex))
	print(sprintf("Time Index: %d", timeindex))
	
	indextable <- unique(datatable[, colindex])
	cnt <- 1
	for (n in seq(length(indextable))) {
			i <- indextable[[n]]
				
			fdata <- subset(datatable, datatable[[ collabel ]] == i, select = c(colindex, timeindex))
			
			print(sprintf("Processing width = %d", i))
			print(sprintf("Number of measurements: %d", nrow(fdata)))
			
			# print(fdata$time.elapsed..seconds.)
			
			avg <- mean(fdata[[ timelabel ]])
			print(avg)
			tdata <- t.test(fdata[[ timelabel ]], mu = 0, alternative="two.sided", conf.level = 0.95)
			avg <- tdata$estimate[[1]]
			confl <- tdata$conf.int[[1]]
			confh <- tdata$conf.int[[2]]
			resultdata[cnt,] <- list(i, avg, confl, confh)
			cnt <- cnt + 1
			print(tdata)
	}
	print(resultdata)
}

# 2-dimensional performance tests

library(scatterplot3d)

filterdata <- function(par1, par2, datatable, devstone) {
	if (devstone)
		return(subset(datatable, (width == par1) & (depth == par2), select = c(width, depth, time.elapsed..seconds. )))
	else
		return(subset(datatable, (nodes == par1) & (atomics.node == par2), select = c(nodes, atomics.node, time.elapsed..seconds. )))
}

#filterdata(2, 3, devstonedata)

dwtuples <- function(datatable) {
	dwtuples <- unique(datatable[, 3:4])
}

processdata <- function(datatable, isdevstone) {
	resultdata <- data.frame(x = numeric(), y = numeric(), cpu = numeric(), conflow = numeric(), confhigh = numeric())
	indextable <- dwtuples(datatable)
	cnt <- 1
	for (n in seq(nrow(indextable))) {
			i <- indextable[[n,1]]
			j <- indextable[[n,2]]
			fdata <- filterdata(i, j, datatable, isdevstone)
			print(sprintf("Processing width = %d and height = %d", i, j))
			print(sprintf("Number of measurements: %d", nrow(fdata)))
			# print(fdata$time.elapsed..seconds.)
			avg <- mean(fdata$time.elapsed..seconds.)
			print(avg)
			tdata <- t.test(fdata$time.elapsed..seconds., mu = 0, alternative="two.sided", conf.level = 0.99)
			avg <- tdata$estimate[[1]]
			confl <- tdata$conf.int[[1]]
			confh <- tdata$conf.int[[2]]
			resultdata[cnt,] <- list(i, j, avg, confl, confh)
			cnt <- cnt + 1
			print(tdata)
	}
	resultdata
}

displaydata <- function(perfdata, isdevstone, label) {
	perfdata <- processdata(perfdata, isdevstone)
	if (isdevstone) {
		par1label <- "Width"
		par2label <- "Height"
	} else {
		par1label <- "Nodes"
		par2label <- "Atomics/Node"
	}	
	
	with(perfdata, {
	   s3d <<- scatterplot3d(x, y, cpu,        # x y and z axis
					 color="blue", pch=19, # filled blue circles
					 type="h",             # lines to the horizontal plane
					 main=label,
					 xlab=par1label,
					 ylab=par2label,
					 zlab="Elapsed CPU Time (seconds)")
					 # xlim = c(1, 5),
					 # ylim = c(0, 4),
					 # zlim = c(1, 1.10),
					 # scale.x = 0.5, scale.y = 0.5,
		s3d$points3d(x, y, conflow, col="red", pch=3, cex=0.5)
		s3d$points3d(x, y, confhigh, col="red", pch=3, cex=0.5)
		# s3d$lines3d(x, y, confhigh - conflow, col="red")
		legend("bottomright", cex=0.7, c("Actual Value", "95 % Confid. Interval"), pch=c(19, 3), col=c("blue", "red"))
	})
	return(s3d)
}

newrange <- function(range1, range2) {
	return(c(min(range1[1], range2[1]), max(range1[2], range2[2])))
}

myrainbow <- function(n) {
	if (n <= 6)
		return(c("red", "green", "blue", "magenta", "dodgerblue", "black" ))
	else
		return(rainbow(n))
}

comparesets <- function(datalist, labellist, xlabel, ylabel, chartlabel, legendpos) {
	print(sprintf("comparesets %s\n",chartlabel))
	nrcharts <- length(datalist)
	
	xrange <- range(datalist[[1]]$x)
	yrange <- range(datalist[[1]]$cpu)
	for (i in 2:nrcharts) {
		xrange <- newrange(xrange, range(datalist[[i]]$x))
		yrange <- newrange(yrange, range(datalist[[i]]$cpu))
	}
	
	if (!generatePDF) {
		postscript(sprintf("%sfig%d.eps", figspath, epscount), width=5, height=5, paper="special", family="Times")
		epscount <<- epscount + 1
	}
	
	if (xrange[[2]] - xrange[[1]] < 10) {
		# special case for CPU's
		plot(xrange, yrange, type = "n", xlab=xlabel, ylab=ylabel, xaxt='n')
		axis(side = 1, at = seq(-10,10,2) , labels = T)
	} else
		plot(xrange, yrange, type = "n", xlab=xlabel, ylab=ylabel)
	
	colors <- myrainbow(nrcharts)
	linetype <- c(1:nrcharts)
	plotchar <- seq(15, 15+nrcharts, 1)
	
	for (i in 1:nrcharts) {
		lines(datalist[[i]]$x, datalist[[i]]$cpu,
			type="b",
			lwd=2,
			lty=linetype[i],
			col=colors[i],
			pch=plotchar[i])
	}
	
	title(chartlabel)
	
	if (legendpos == "topright") {
		legendxpos = xrange[2]
		legendypos = yrange[2]
		legendxjust = 1.0
		legendyjust = 1.0
	} else if (legendpos == "midleft") {
		legendxpos = xrange[1]
		legendypos = (yrange[1] + yrange[2])/2
		legendxjust = 0.0
		legendyjust = 0.5
	} else {	# "topleft"
		legendxpos = xrange[1]
		legendypos = yrange[2]
		legendxjust = 0.0
		legendyjust = 1.0
	}
	
	legend(legendxpos, legendypos,
		xjust = legendxjust, yjust = legendyjust,
		labellist,
		cex=0.8,
		col=colors,
		pch=plotchar,
		lty=linetype,
		title="Legend")
	
	if (!generatePDF)
		dev.off()	
	
	lateXTable(datalist, labellist, xlabel, ylabel, chartlabel)
}

compareNsets <- function(datalist, labellist, xlabel, ylabel, chartlabel, devstone, legendpos) {
	print("=================== compareNsets ===================")
	N <- length(datalist)
	perflist <- list(N)
	for (i in 1:N) {
		perfdata <- process1ddata(datalist[[i]], devstone)
		perflist[[i]] <- perfdata
	}
	print(perflist[[1]]$x)
	comparesets(perflist, labellist, xlabel, ylabel, chartlabel, legendpos)
}

lateXInit <- function(filename) {
	lateXFilename <<- filename
	s <- "\\documentclass[a4paper, 11pt]{article}\n\\begin{document}\n"
	write(s, file=lateXFilename, append=FALSE, sep="\n")
}

lateXExit <- function() {
	s <- "\\end{document}\n"
	write(s, file=lateXFilename, append=TRUE, sep="")
}

lateXTable <- function(datalist, labellist, xlabel, ylabel, chartlabel) {
	nrcharts <- length(datalist)
	firstchart = datalist[[1]]
	
	s <- "\\begin{figure}\n\\begin{center}\n\\begin{tabular}{|c|"
	for (i in 1:nrcharts)
		s <- paste(s, "r|", sep="")
	s <- paste(s, "}\n\\hline", sep="")
	
	write(s, file=lateXFilename, append=TRUE, sep="\n")
	s <- xlabel
	for (i in 1:nrcharts)
		s <- paste(s, labellist[[i]], sep="&")
	s <- paste(s, "\\\\\n\\hline")
	write(s, file=lateXFilename, append=TRUE, sep="\n")
	for (i in 1:length(datalist[[1]]$x)) {
		s <- sprintf("%.0f", datalist[[1]]$x[i])
		for (j in 1:nrcharts) {
			s <- paste(s, sprintf("%.4f",datalist[[j]]$cpu[i]), sep="&")
		}
		s <- paste(s, "\\\\", sep="")
		write(s, file=lateXFilename, append = TRUE, sep = "\n")
	}
	
	s <- "\\hline\n\\end{tabular}\n\\end{center}\n"
	s <- paste(s, sprintf("\\caption{%s}\n\\end{figure}\n", chartlabel), sep="")
	write(s, file=lateXFilename, append=TRUE, sep="")
}

gengraphs <- function() {
	dxdevstonedata = read.table(file="devstone/classic.csv",header=TRUE,sep=";")
	adevstonedata = read.table(file="adevstone/classic.csv",header=TRUE,sep=";")
	adevstoneconsdata = read.table(file="adevstone/conservative.csv",header=TRUE,sep=";")
	dxdevstoneconsdata = read.table(file="devstone/conservative.csv",header=TRUE,sep=";")
	dxdevstoneoptdata = read.table(file="devstone/optimistic.csv",header=TRUE,sep=";")
	
	dxconnectconsdata = read.table(file="connect/conservative.csv",header=TRUE,sep=";")
	adevsconnectconsdata = read.table(file="aconnect/conservative.csv",header=TRUE,sep=";")
	dxconnectdata = read.table(file="connect/classic.csv",header=TRUE,sep=";")
	adevsconnectdata = read.table(file="aconnect/classic.csv",header=TRUE,sep=";")
	dxconnectdata2cores <- subset(dxconnectconsdata, ncores == 2)
	dxconnectdata4cores <- subset(dxconnectconsdata, ncores == 4) 
	adevsconnectdata2cores <- subset(adevsconnectconsdata, ncores == 2) 
	adevsconnectdata4cores <- subset(adevsconnectconsdata, ncores == 4) 
	
	dxpholddata = read.table(file="phold/classic.csv",header=TRUE,sep=";")
	apholddata = read.table(file="aphold/classic.csv",header=TRUE,sep=";")
	dxpholdconsdata = read.table(file="phold/conservative.csv",header=TRUE,sep=";")
	apholdconsdata = read.table(file="aphold/conservative.csv",header=TRUE,sep=";")
	dxpholdoptdata = read.table(file="phold/optimistic.csv",header=TRUE,sep=";")
	
	dxpriorityconsdata = read.table(file="priority/conservative.csv",header=TRUE,sep=";")
	dxpriorityoptdata = read.table(file="priority/optimistic.csv",header=TRUE,sep=";")
	
	lateXInit(paste(figspath, 'DXvsADEVS.tex', sep=''))
	
	if (generatePDF)
		pdf(paste(figspath, 'DXvsADEVS.pdf', sep=''), family="Times")
	# jpeg("d:/tmp/DXvsADEVS.jpg", width=5, height=5, units="in", res=300)
	
	if (!generatePDF)
		setEPS()
		
	compareNsets(
		list(dxdevstonedata, adevstonedata),
		list("dxex", "adevs"),
		"Width/Depth", "Elapsed Time (sec.)", "Devstone single core", "width", "topleft")
	
	compareNsets(
		list(dxdevstoneoptdata,
			dxdevstoneconsdata,
			adevstoneconsdata),
		list("dxex optimistic",
			"dxex conservative",
			"adevs conservative"),
		"Width/Depth", "Elapsed Time (sec.)", "DevStone parallel", "width", "topleft")
	
	compareNsets(
		list(dxconnectdata,
			adevsconnectdata,
			dxconnectdata2cores,
			dxconnectdata4cores,
			adevsconnectdata2cores,
			adevsconnectdata4cores),
		list("dxex single core",
			"adevs single core",
			"dxex conservative (2 cores)",
			"dxex conservative (4 cores)",
			"adevs conservative (2 cores)",
			"adevs conservative (4 cores)"),
		"Width", "Elapsed Time (sec.)", "Interconnect", 'width', "topleft")
	
	compareNsets(
		list(dxpholddata,
			apholddata,
			dxpholdconsdata,
			apholdconsdata,
			dxpholdoptdata),
		list("dxex single core",
			"adevs single core",
			"dxex conservative",
			"adevs conservative",
			"dxex optimistic"),
		"Atomics/Node", "Elapsed Time (sec.)", "Phold", "atomics.node", "topleft")
	
	compareNsets(
		list(dxpriorityconsdata, dxpriorityoptdata),
		list("dxex conservative", "dxex optimistic"),
		"Messages", "Elapsed Time (sec.)", "Priority", "messages", "topleft")
	
	if (generatePDF)
		dev.off()
		
	lateXExit()
}

group4 <- function() {
	print(sprintf("Group4: datapath=%s", datapath))
	generatePDF <<- TRUE # generate PDF
	gengraphs()
	generatePDF <<- FALSE # generate EPS files
	gengraphs()
}

group4()



