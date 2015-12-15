/*
 * This file is part of the DEVS Ex Machina project.
 * Copyright 2014 - 2015 University of Antwerp
 * https://www.uantwerpen.be/en/
 * Licensed under the EUPL V.1.1
 * A full copy of the license is in COPYING.txt, or can be found at
 * https://joinup.ec.europa.eu/community/eupl/og_page/eupl
 *      Author: Ben Cardoen
 */

#include <mutex>

#include "tools/gviz.h"
#include "model/core.h"
#include <string>

n_tools::GVizWriter::GVizWriter(const std::string& outname):m_file(outname)
{
        m_file.exceptions(std::ofstream::failbit);
        writeHeader();
}

n_tools::GVizWriter* 
n_tools::GVizWriter::getWriter(const std::string fname){
        static n_tools::GVizWriter* writer = nullptr;    // Don't capture statics.
        static std::once_flag flag_singleton;
        // Creation can throw, but we can't recover anyway. Note that call_once forwards exception and tries again for the next invocation.
        auto factory = [fname](){
                        writer=new n_tools::GVizWriter(fname);
        };
        std::call_once(flag_singleton, factory);
        return writer;
}

void
n_tools::GVizWriter::writeHeader(){
        std::lock_guard<std::mutex>l(this->m_mu);
        if(!m_file.is_open())
                throw std::logic_error("File not open");
        m_file << "digraph G { \n";
        m_file << "compound=true;\n";
        m_file<< "fontsize=10;\n";
}

void
n_tools::GVizWriter::writeObject(n_model::Core* core){
        
        std::lock_guard<std::mutex>l(this->m_mu);
        m_file << "subgraph cluster_"<< core->getCoreID()<<" { \n";
        m_file << "style=filled;\n";
	m_file<< "color=lightgrey;\n";
        m_file<< "fontsize=9;\n";
        m_file << "\tlabel = \"Core " << core->getCoreID() << " \";\n";
        for(const auto& model : core->m_indexed_models){
                m_file << '\t'<<model->getName() << " [fontsize=10]  ;\n";
        }
        m_file << "\t}\n";

        for(const auto& model : core->m_indexed_models){
                std::string from = model->getName();
                for(const auto& oport : model->getOPorts()){
                        for(const auto& pc : oport->m_sendstat){
#ifdef USE_STAT                                
                                auto count = pc.second.getData();
#else
                                auto count = 0;
#endif
                                t_portptr_raw dport= pc.first;
                                AtomicModel_impl* model = dynamic_cast<AtomicModel_impl*>(dport->getHost());
                                std::string to = model->getName();
                                m_file << from << " -> " << to;
                                if(count){
                                        std::string color = "blue";
                                        if(model->getCoreID()!=core->getCoreID()) // intercore
                                                m_file<<" [ label=\""<< count <<"\"pendwidth=0.5 , color=red, fontsize=7.0 ]";
                                        else //intracore
                                                m_file<<" [ label=\""<<count<<"\" penwidth=0.5, color=blue,fontsize=7.0 ]"; 
                                }else{ // never sent
                                        m_file<<" [ style=\"dotted\", pendwidth=0.1, arrowhead=\"none\"]";
                                }
                                m_file << ";\n";
                        }
                }
        }
}

n_tools::GVizWriter::~GVizWriter(){
        std::lock_guard<std::mutex>l(this->m_mu);
        try{
                // if this fails, there is no way to recover. But we're in a destructor, so we can't throw.
                writeFooter();
        }
        catch(...){;}
}

void
n_tools::GVizWriter::writeFooter(){
        m_file << "}\n";
        m_file.close();
}

