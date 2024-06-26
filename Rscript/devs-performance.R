# global variables
generatePDF <<- FALSE	# if TRUE, generate PDF else: generated EPS files...
epscount <<- 1
rootpath <<- paste(getwd(), '/', sep = '')
# rootpath <<- 'putinwhateveryoulike';
datapath <<- paste(rootpath, 'data/', sep = '')
figspath <<- paste(rootpath, 'figs/', sep = '')
dir.create(figspath, showWarnings = FALSE)
setwd(datapath)

# 1-dimensional performance tests

process1ddata <- function(datatable, collabel) {
	resultdata <- data.frame(x = numeric(), cpu = numeric(), conflow = numeric(), confhigh = numeric())
		
	timelabel = 'time.elapsed..seconds.'

	colindex = which(colnames(datatable) == collabel)
	timeindex = which(colnames(datatable) == timelabel)
	
	indextable <- unique(datatable[, colindex])
	cnt <- 1
	for (n in seq(length(indextable))) {
			i <- indextable[[n]]
				
			fdata <- subset(datatable, datatable[[ collabel ]] == i, select = c(colindex, timeindex))
			
			avg <- mean(fdata[[ timelabel ]])
			tdata <- t.test(fdata[[ timelabel ]], mu = 0, alternative="two.sided", conf.level = 0.95)
			avg <- tdata$estimate[[1]]
			confl <- tdata$conf.int[[1]]
			confh <- tdata$conf.int[[2]]
			resultdata[cnt,] <- list(i, avg, confl, confh)
			cnt <- cnt + 1
	}
	return(resultdata)
}

filterdata <- function(par1, par2, datatable, devstone) {
	if (devstone)
		return(subset(datatable, (width == par1) & (depth == par2), select = c(width, depth, time.elapsed..seconds. )))
	else
		return(subset(datatable, (nodes == par1) & (atomics.node == par2), select = c(nodes, atomics.node, time.elapsed..seconds. )))
}

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
			avg <- mean(fdata$time.elapsed..seconds.)
			tdata <- t.test(fdata$time.elapsed..seconds., mu = 0, alternative="two.sided", conf.level = 0.99)
			avg <- tdata$estimate[[1]]
			confl <- tdata$conf.int[[1]]
			confh <- tdata$conf.int[[2]]
			resultdata[cnt,] <- list(i, j, avg, confl, confh)
			cnt <- cnt + 1
	}
	return(resultdata)
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
	if (n == 2)
		return(c("magenta", "dodgerblue"))
	else if (n <= 6)
		return(c("red", "green", "blue", "magenta", "dodgerblue", "black" ))
	else
		return(rainbow(n))
}

myplotchars <- function(n) {
	if (n == 2)
		return(c(18, 22))
	else if (n <= 8)
		return(c(15, 17, 19, 18, 22, 24, 21, 5))
	else
		return(seq(15, 15+n, 1))
}

mylinetype <- function(n) {
	return(rep(1,n)) 
	if (FALSE) {
		if (n == 2)
			return(c(4,5))
		else
			return(c(1:n))
	}
}

formatlabels <- function(xvalues, formattype) {
	if (formattype == "cartesian") {
		formattedvalues <- array()
		for (i in 1:length(xvalues)) {
			formattedvalues[[i]] <- sprintf("%0d",round(as.numeric(xvalues[[i]])*as.numeric(xvalues[[i]])))
		}
		return(formattedvalues)
	}
	else
		return(xvalues);
}

newxlabels <- function(minmax) {
	delta <- minmax[2] - minmax[1] + 1
	deltas <- c(1,2,5,10,25,50,100,250,500,1000,10000)
	for (i in 1:length(deltas)) {
		if (delta/deltas[i] < 10)
			return(seq(minmax[1], minmax[2], deltas[i]))
	}
	return(seq(minmax[1],minmax[2],100000))
}

comparesets <- function(filename, datalist, labellist, xlabel, ylabel, chartlabel, formatlabel, legendpos) {
	nrcharts <- length(datalist)
	
	xrange <- range(datalist[[1]]$x)
	yrange <- range(datalist[[1]]$cpu)
	for (i in 2:nrcharts) {
		xrange <- newrange(xrange, range(datalist[[i]]$x))
		yrange <- newrange(yrange, range(datalist[[i]]$cpu))
	}
	
	if (!generatePDF) {
		postscript(sprintf("%s%s.eps", figspath, filename), width=5, height=5, paper="special", family="Times")
		epscount <<- epscount + 1
	}

	if (!generateTitles) {
		margins <- par("mai")
		margins <- margins - c(0, 0, margins[3] - 0.1, 0)
		par(mai = margins)
	}
	
	newlabels <- newxlabels(xrange)
	plot(xrange, yrange, type = "n", xlab=xlabel, ylab=ylabel, xaxt='n')
	axis(side = 1, at = newlabels , labels = formatlabels(newlabels, formatlabel))
	
	colors <- myrainbow(nrcharts)
	linetype <- mylinetype(nrcharts)
	plotchar <- myplotchars(nrcharts)
	
	for (i in 1:nrcharts) {
		lines(datalist[[i]]$x, datalist[[i]]$cpu,
			type="b",
			lwd=2,
			lty=linetype[i],
			col=colors[i],
			pch=plotchar[i])
	}
	
	if (generateTitles)
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

compareNsets <- function(filename, datalist, labellist, xlabel, ylabel, chartlabel, fieldname, formatlabel, legendpos) {
	print(sprintf("... Generating %s", chartlabel))
	N <- length(datalist)
	perflist <- list(N)
	for (i in 1:N) {
		perfdata <- process1ddata(datalist[[i]], fieldname)
		perflist[[i]] <- perfdata
	}
	comparesets(filename, perflist, labellist, xlabel, ylabel, chartlabel, formatlabel, legendpos)
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

speedup <- function(multicoredata, singlecoredata) {
	average <- mean(singlecoredata$time.elapsed..seconds.)
	multicoredata$time.elapsed..seconds. <- average / multicoredata$time.elapsed..seconds.
	return(multicoredata)
}

speedup2 <- function(multicoredata, singlecoredata, field) {
	nrrows <- dim(multicoredata)[1]
	for (i in 1:nrrows) {
		value <- multicoredata[[field]][i]
		sprintf("%d %d", i, value)
		datasubset <- subset(singlecoredata, singlecoredata[[field]] == value)
		average = mean(datasubset$time.elapsed..seconds.)
		multicoredata$time.elapsed..seconds.[i] <- average / multicoredata$time.elapsed..seconds.[i]
	}
	return(multicoredata)
}

addModels <- function(data) {
	nrrows <- dim(data)[1]
	data$models <- 0
	for (i in 1:nrrows) {
		data$models[i] <- data$width[i] * data$depth[i]
	}
	return(data)
}

gengraphs <- function() {
	lateXInit(paste(figspath, 'DXvsADEVS.tex', sep=''))
	
	if (generatePDF)
		pdf(paste(figspath, 'DXvsADEVS.pdf', sep=''), family="Times")
	# jpeg("d:/tmp/DXvsADEVS.jpg", width=5, height=5, units="in", res=300)
	
	if (!generatePDF)
		setEPS()
	
	# Figure 1: single core dxex/adevs in function of varying width/depth
	
	dxdevstonedata = read.table(file="devstone/classic.csv",header=TRUE,sep=";")
	dxdevstonedata <- addModels(dxdevstonedata)
	adevstonedata = read.table(file="adevstone/classic.csv",header=TRUE,sep=";")
	adevstonedata <- addModels(adevstonedata)
	
	compareNsets(
		"queue_sequential",
		list(dxdevstonedata, adevstonedata),
		list("dxex single core", "adevs single core"),
		"Models", "Elapsed Time (sec.)", "Devstone single core", "models", "normal", "topleft")

	# Figure 2: speed-up multi-core for width/depth = 30
	adevstoneconsdata <- subset(read.table(file="adevstone/conservative.csv",header=TRUE,sep=";"), width == 30)
	dxdevstoneconsdata <- subset(read.table(file="devstone/conservative.csv",header=TRUE,sep=";"), width == 30)
	dxdevstoneoptdata <- subset(read.table(file="devstone/optimistic.csv",header=TRUE,sep=";"), width == 30)
	adevstoneconsdata_speedup <- speedup(adevstoneconsdata, subset(adevstonedata, width == 30))
	dxdevstoneconsdata_speedup <- speedup(dxdevstoneconsdata, subset(dxdevstonedata, width == 30))
	dxdevstoneoptdata_speedup <- speedup(dxdevstoneoptdata, subset(dxdevstonedata, width == 30))
	
	compareNsets(
		"queue_parallel",
		list(dxdevstoneoptdata_speedup,
			dxdevstoneconsdata_speedup,
			adevstoneconsdata_speedup),
		list("dxex optimistic",
			"dxex conservative",
			"adevs conservative"),
		"Cores", "Speedup", "DevStone parallel", "ncores", "normal", "topleft")

	# Figure 3: single core Interconnect
	dxconnectdata = read.table(file="connect/classic.csv",header=TRUE,sep=";")
	adevsconnectdata = read.table(file="aconnect/classic.csv",header=TRUE,sep=";")
	
	compareNsets(
		"interconnect_sequential",
		list(dxconnectdata,
			adevsconnectdata),
		list("dxex single core",
			"adevs single core"),
		"Models", "Elapsed Time (sec.)", "Interconnect single core", 'width', "normal", "topleft")
	
	# Figure 4: speed-up multi-core for width = 8
	dxconnectsinglecoredata = read.table(file="connect_speedup/classic.csv",header=TRUE,sep=";")
	adevsconnectsinglecoredata = read.table(file="aconnect_speedup/classic.csv",header=TRUE,sep=";")
	dxconnectconsdata = speedup(read.table(file="connect_speedup/conservative.csv",header=TRUE,sep=";"), dxconnectsinglecoredata)
	dxconnectoptdata = speedup(read.table(file="connect_speedup/optimistic.csv",header=TRUE,sep=";"), dxconnectsinglecoredata)
	adevsconnectconsdata = speedup(read.table(file="aconnect_speedup/conservative.csv",header=TRUE,sep=";"), adevsconnectsinglecoredata)
	
	compareNsets(
		"interconnect_parallel",
		list(dxconnectoptdata,
			dxconnectconsdata,
			adevsconnectconsdata),
		list("dxex optimistic",
			"dxex conservative",
			"adevs conservative"),
		"Cores", "Speedup", "Interconnect parallel", "ncores", "normal", "topright")
		
	# Figure 5: speed-up multi-core priority with varying priority
	dxpholddata = read.table(file="phold/classic.csv",header=TRUE,sep=";")
	apholddata = read.table(file="aphold/classic.csv",header=TRUE,sep=";")
	dxpholdconsdata = speedup2(read.table(file="phold/conservative.csv",header=TRUE,sep=";"), dxpholddata, 'X..priority')
	apholdconsdata = speedup2(read.table(file="aphold/conservative.csv",header=TRUE,sep=";"), apholddata, 'X..priority')
	dxpholdoptdata = speedup2(read.table(file="phold/optimistic.csv",header=TRUE,sep=";"), dxpholddata, 'X..priority')
	compareNsets(
		"phold_speedup_priority",
		list(dxpholdoptdata,
			dxpholdconsdata,
			apholdconsdata),
		list("dxex optimistic",
			"dxex conservative",
			"adevs conservative"),
		"% Priority", "Speedup", "Phold parallel", "X..priority", "normal", "topright")
		
	# Figure 6: speed-up multi-core priority with varying remotes
	dxpholddata = read.table(file="phold_remotes/classic.csv",header=TRUE,sep=";")
	apholddata = read.table(file="aphold_remotes/classic.csv",header=TRUE,sep=";")
	dxpholdconsdata = speedup2(read.table(file="phold_remotes/conservative.csv",header=TRUE,sep=";"), dxpholddata, 'X..remotes')
	apholdconsdata = speedup2(read.table(file="aphold_remotes/conservative.csv",header=TRUE,sep=";"), apholddata, 'X..remotes')
	dxpholdoptdata = speedup2(read.table(file="phold_remotes/optimistic.csv",header=TRUE,sep=";"), dxpholddata, 'X..remotes')
	compareNsets(
		"phold_speedup_remotes",
		list(dxpholdoptdata,
			dxpholdconsdata,
			apholdconsdata),
		list("dxex optimistic",
			"dxex conservative",
			"adevs conservative"),
		"% Remotes", "Speedup", "Phold parallel", "X..remotes", "normal", "topright")
	
	if (generatePDF)
		dev.off()
		
	lateXExit()
}

group5 <- function() {
	print(sprintf("rootpath=%s", rootpath))
	print(sprintf("datapath=%s", datapath))
	
	print("Generating PDF")
	generatePDF <<- TRUE # generate PDF
	generateTitles <<- TRUE
	gengraphs()
	
	print("Generating EPS files")
	generatePDF <<- FALSE # generate EPS files
	generateTitles <<- FALSE
	gengraphs()
	
	print("Done")
}

group5()



