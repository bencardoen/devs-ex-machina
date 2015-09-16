script.dir <- dirname(sys.frame(1)$ofile)
setwd(script.dir)

# 1-dimensional performance tests

process1ddata <- function(datatable) {
	resultdata <- data.frame(x = numeric(), cpu = numeric(), conflow = numeric(), confhigh = numeric())
	indextable <- unique(datatable[, 3])
	cnt <- 1
	for (n in seq(length(indextable))) {
			i <- indextable[[n]]

			fdata <- subset(datatable, width == i, select = c(width, time.elapsed..seconds. ))
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

	filename <- paste(script.dir, '/myconnectgraphs.pdf', sep="")
	pdf(filename)
	
	perfdata1 <- process1ddata(aconnectdata)
	display1ddata(perfdata1, "A Connect")
	display1ddata2(aconnectdata, "A Connect")
	display1ddata3(aconnectdata, perfdata1, "A Connect")
	
	perfdata2 <- process1ddata(dxconnectdata)
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

group2 <- function() {
	adevstonedata = read.table(file="adevstone_classic.csv",header=TRUE,sep=";")
	dxdevstonedata = read.table(file="dxdevstone_classic.csv",header=TRUE,sep=";")
	apholddata = read.table(file="aphold_classic.csv",header=TRUE,sep=";")
	dxpholddata = read.table(file="dxphold_classic.csv",header=TRUE,sep=";")

	filename <- paste(script.dir, '/mygraphs.pdf', sep="")
	pdf(filename)
	displaydata(dxdevstonedata, TRUE, "DX DevStone")
	displaydata(adevstonedata, TRUE, "A DevStone")
	displaydata(dxpholddata, FALSE, "DX PHold")
	displaydata(apholddata, FALSE, "A PHold")
	displaydata2(dxdevstonedata, "DX DevStone", adevstonedata, "A DevStone", TRUE, "DX DevStone vs. A DevStone")
	displaydata2(dxpholddata, "DX PHold", apholddata, "A PHold", FALSE, "DX PHold vs. A PHold")
	dev.off()
}



