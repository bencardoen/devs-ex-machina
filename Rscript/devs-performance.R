# Change this path to the folder all .csv files are stored per benchmark
# This will become dynamic in the future, still being tested on different platforms
setwd("d:/tim/ua/statistiek/tim-eindwerk/v6/data")

# 1-dimensional performance tests

process1ddata <- function(datatable, devstone) {
	resultdata <- data.frame(x = numeric(), cpu = numeric(), conflow = numeric(), confhigh = numeric())
	indextable <- unique(datatable[, 3])
	cnt <- 1
	for (n in seq(length(indextable))) {
			i <- indextable[[n]]

			if (devstone)
				fdata <- subset(datatable, width == i, select = c(width, time.elapsed..seconds. ))
			else
				fdata <- subset(datatable, nodes == i, select = c(nodes, time.elapsed..seconds. ))
			print(sprintf("Processing width = %d", i))
			print(sprintf("Number of measurements: %d", nrow(fdata)))
			# print(fdata$time.elapsed..seconds.)
			avg <- mean(fdata$time.elapsed..seconds.)
			print(avg)
			tdata <- t.test(fdata$time.elapsed..seconds., mu = 0, alternative="two.sided", conf.level = 0.95)
			avg <- tdata$estimate[[1]]
			confl <- tdata$conf.int[[1]]
			confh <- tdata$conf.int[[2]]
			resultdata[cnt,] <- list(i, avg, confl, confh)
			cnt <- cnt + 1
			print(tdata)
	}
	print(resultdata)
}

display1ddata <- function(perfdata, label) {
	plot(perfdata$x, perfdata$cpu, pch=19, xlab="Width", ylab="Elapsed CPU Time (seconds)", main=label, col="blue")
	abline(lm(perfdata$x ~ perfdata$cpu), col="red", lwd=2, lty=1)
}

display1ddata2 <- function(singledata, label) {
	boxplot(time.elapsed..seconds. ~ width, data=singledata, xlab="Width", ylab="Elapsed CPU Time (seconds)", main=label)
}

display1ddata3 <- function(singledata, perfdata, label) {
	indextable <- unique(singledata[, 3])
	for (n in seq(length(indextable))) {
			i <- indextable[[n]]
			tmpdata <- singledata$time.elapsed..seconds.[singledata$width == i]
			boxplot(tmpdata, xlab=sprintf("Width = %d", i), ylab="Elapsed CPU Time (seconds)", main=label)
	}
}

group1 <- function() {
	aconnectdata = read.table(file="aconnect_classic.csv",header=TRUE,sep=";")
	dxconnectdata = read.table(file="dxconnect_classic.csv",header=TRUE,sep=";")

	pdf("d:/tmp/myconnectgraphs.pdf")
	
	perfdata1 <- process1ddata(aconnectdata, TRUE)
	display1ddata(perfdata1, "A Connect")
	display1ddata2(aconnectdata, "A Connect")
	display1ddata3(aconnectdata, perfdata1, "A Connect")
	
	perfdata2 <- process1ddata(dxconnectdata, TRUE)
	display1ddata(perfdata2, "DX Connect")
	display1ddata2(dxconnectdata, "DX Connect")
	display1ddata3(dxconnectdata, perfdata2, "DX Connect")
	
	dev.off()
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

displaydata2 <- function(perfdata1, label1, perfdata2, label2, isdevstone, label) {
	perfdata1 <- processdata(perfdata1, isdevstone)
	perfdata1$color = "blue"
	perfdata2 <- processdata(perfdata2, isdevstone)
	perfdata2$color = "green"
	perfdata <- rbind(perfdata1, perfdata2)
	if (isdevstone) {
		par1label <- "Width"
		par2label <- "Height"
	} else {
		par1label <- "Nodes"
		par2label <- "Atomics/Node"
	}
	
	minx <- min(perfdata$x)
	maxx <- max(perfdata$x)
	miny <- min(perfdata$y)
	maxy <- max(perfdata$y)
	
	with(perfdata, {
	   s3d <<- scatterplot3d(x, y, cpu,  # x y and z axis
					 pch=19, 			# filled circles
					 type="h",          # lines to the horizontal plane
					 main=label,
					 color=perfdata$color,
					 xlim = c(minx, maxx),
					 ylim = c(miny, maxy),
					 xlab=par1label,
					 ylab=par2label,
					 zlab="Elapsed CPU Time (seconds)")
					 # zlim = c(1, 1.10),
					 # scale.x = 0.5, scale.y = 0.5,
		legend("topleft", cex=0.7, c(label1, label2), pch=c(19, 19), col=c("blue", "green"))
	})
	return(s3d)
}

calccorrelation <- function(data1, label1, data2, label2, isdevstone) {
	if (isdevstone)
		tmpdata <- cbind(data1[,c(3,4,26)],data2[,c(26)])
	else
		tmpdata <- cbind(data1[,c(3,4,28)],data2[,c(28)])
	names(tmpdata)[3] <- "cpu"
	names(tmpdata)[4] <- "cpu2"
	print(tmpdata)
	#cor.test(tmpdata[,3], tmpdata[,4])
	fit <- lm(cpu2 ~ cpu, data = tmpdata)
	summary(fit)
	plot(tmpdata$cpu, tmpdata$cpu2, main=sprintf("Elapsed CPU Time (seconds) %s vs. %s", label1, label2), xlab="", ylab="", xaxt="n", yaxt="n")
	axis(side=1)
	axis(side=2)
	title(xlab=label1, ylab=label2)
	abline(fit, col="red", lwd=2)
}

group2 <- function() {
	adevstonedata = read.table(file="adevstone/classic.csv",header=TRUE,sep=";")
	dxdevstonedata = read.table(file="devstone/classic.csv",header=TRUE,sep=";")
	adevstoneconsdata = read.table(file="adevstone/conservative.csv",header=TRUE,sep=";")
	dxdevstoneconsdata = read.table(file="devstone/conservative.csv",header=TRUE,sep=";")

	pdf("d:/tmp/mygraphs.pdf")
	displaydata(dxdevstonedata, TRUE, "DX DevStone Classic")
	displaydata(adevstonedata, TRUE, "A DevStone Classic")
	displaydata(dxdevstoneconsdata, TRUE, "DX DevStone Conservative")
	displaydata(adevstoneconsdata, TRUE, "A DevStone Conservative")
	displaydata2(dxdevstonedata, "DX DevStone Classic", adevstonedata, "A DevStone Classic", TRUE, "DX DevStone Classic vs. A DevStone Classic")
	displaydata2(dxdevstoneconsdata, "DX DevStone Conservative", adevstoneconsdata, "A DevStone Classic", TRUE, "DX DevStone Conservative vs. A DevStone Conservative")
	calccorrelation(dxdevstonedata, "DX DevStone Classic", adevstonedata, "A DevStone Classic", TRUE)
	calccorrelation(dxdevstoneconsdata, "DX DevStone Conservative", adevstoneconsdata, "A DevStone Classic", TRUE)
	dev.off()
}

newrange <- function(range1, range2) {
	return(c(min(range1[1], range2[1]), max(range1[2], range2[2])))
}

comparesets <- function(datalist, labellist, xlabel, ylabel, chartlabel) {
	nrcharts <- length(datalist)
	
	xrange <- range(datalist[[1]]$x)
	yrange <- range(datalist[[1]]$cpu)
	for (i in 2:nrcharts) {
		xrange <- newrange(xrange, range(datalist[[i]]$x))
		yrange <- newrange(yrange, range(datalist[[i]]$cpu))
	}
	
	plot(xrange, yrange, type = "n", xlab=xlabel, ylab=ylabel)
	
	colors <- rainbow(nrcharts)
	linetype <- c(1:nrcharts)
	plotchar <- seq(18, 18+nrcharts, 1)
	
	for (i in 1:nrcharts) {
		lines(datalist[[i]]$x, datalist[[i]]$cpu,
			type="b",
			lwd=2,
			lty=linetype[i],
			col=colors[i],
			pch=plotchar[i])
	}
	
	title(chartlabel)
	
	legend(xrange[1], yrange[2],
		labellist,
		cex=0.8,
		col=colors,
		pch=plotchar,
		lty=linetype,
		title="Legend")
		
	lateXTable(datalist, labellist, xlabel, ylabel, chartlabel)
}

compare2sets <- function(data1, label1, data2, label2, xlabel, ylabel, chartlabel, devstone) {
	perfdata1 <- process1ddata(data1, devstone)
	perfdata2 <- process1ddata(data2, devstone)
	datalist <- list(perfdata1, perfdata2)
	labellist <- list(label1, label2)
	comparesets(datalist, labellist, xlabel, ylabel, chartlabel)
}

compare3sets <- function(data1, label1, data2, label2, data3, label3, xlabel, ylabel, chartlabel, devstone) {
	perfdata1 <- process1ddata(data1, devstone)
	perfdata2 <- process1ddata(data2, devstone)
	perfdata3 <- process1ddata(data3, devstone)
	datalist <- list(perfdata1, perfdata2, perfdata3)
	labellist <- list(label1, label2, label3)
	comparesets(datalist, labellist, xlabel, ylabel, chartlabel)
}

group3 <- function() {
	dxdevstonedata = read.table(file="devstone/classic.csv",header=TRUE,sep=";")
	adevstonedata = read.table(file="adevstone/classic.csv",header=TRUE,sep=";")
	adevstoneconsdata = read.table(file="adevstone/conservative.csv",header=TRUE,sep=";")
	dxdevstoneconsdata = read.table(file="devstone/conservative.csv",header=TRUE,sep=";")
	dxdevstoneoptdata = read.table(file="devstone/optimistic.csv",header=TRUE,sep=";")
	
	randdxdevstonedata = read.table(file="randdevstone/classic.csv",header=TRUE,sep=";")
	randadevstonedata = read.table(file="aranddevstone/classic.csv",header=TRUE,sep=";")
	randadevstoneconsdata = read.table(file="aranddevstone/conservative.csv",header=TRUE,sep=";")
	# randdxdevstoneconsdata = read.table(file="randdevstone/conservative.csv",header=TRUE,sep=";")
	randdxdevstoneoptdata = read.table(file="randdevstone/optimistic.csv",header=TRUE,sep=";")
	
	dxconnectdata = read.table(file="connect/classic.csv",header=TRUE,sep=";")
	aconnectdata = read.table(file="aconnect/classic.csv",header=TRUE,sep=";")
	randdxconnectdata = read.table(file="randconnect/classic.csv",header=TRUE,sep=";")
	randaconnectdata = read.table(file="arandconnect/classic.csv",header=TRUE,sep=";")
	
	dxpholddata = read.table(file="phold/classic.csv",header=TRUE,sep=";")
	apholddata = read.table(file="aphold/classic.csv",header=TRUE,sep=";")

	lateXInit("d:/tmp/DXvsADEVS.tex")
	pdf("d:/tmp/DXvsADEVS.pdf")
	
	compare2sets(dxdevstonedata, "DX", adevstonedata, "ADEVS", "Width/Height", "Elapsed Time (sec.)", "DX vs. ADEVS DevStone Single Core", TRUE)
	compare2sets(dxdevstoneconsdata, "DX", adevstoneconsdata, "ADEVS", "Width/Height", "Elapsed Time (sec.)", "DX vs. ADEVS DevStone Conservative", TRUE)
	compare3sets(dxdevstonedata, "Single Core",
		dxdevstoneconsdata, "Conservative",
		dxdevstoneoptdata, "Optimistic",
		"Width/Height", "Elapsed Time (sec.)", "DX DevStone", TRUE)
	
	compare2sets(randdxdevstonedata, "DX", randadevstonedata, "ADEVS", "Width/Height", "Elapsed Time (sec.)", "Random DX vs. ADEVS DevStone Single Core", TRUE)
	# compare2sets(randdxdevstoneconsdata, "DX", randadevstoneconsdata, "ADEVS", "Width", "Elapsed Time (sec.)", "Random DX vs. ADEVS DevStone Conservative", TRUE)
	compare2sets(randdxdevstonedata, "Single Core",
		randdxdevstoneoptdata, "Optimistic",
		"Width/Height", "Elapsed Time (sec.)", "Random DX DevStone", TRUE)
	
	compare2sets(dxconnectdata, "DX",
		aconnectdata, "ADEVS",
		"Width", "Elapsed Time (sec.)", "DX vs. ADEVS Connect Single Core", TRUE)
	compare2sets(randdxconnectdata, "DX",
		randaconnectdata, "ADEVS",
		"Width", "Elapsed Time (sec.)", "DX vs. ADEVS Random Connect Single Core", TRUE)
		
	compare2sets(dxpholddata, "DX",
		apholddata, "ADEVS",
		"Nodes", "Elapsed Time (sec.)", "DX vs. ADEVS PHold Classic", FALSE)
	
	dev.off()
	lateXExit()
}

lateXFilename <<- "d:/tmp/default.tex"

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



