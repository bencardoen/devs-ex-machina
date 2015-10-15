#include <mutex>

#include "tools/gviz.h"
#include "model/core.h"
#include <string>

n_tools::GVizWriter::GVizWriter(const std::string& outname):m_file(outname)
                                ,m_config_node(" [ shape=\"circle\", width=1, height = 1, fixedsize=true];\n")
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
}

void
n_tools::GVizWriter::writeObject(n_model::Core* core){
        std::lock_guard<std::mutex>l(this->m_mu);
        m_file << "subgraph cluster_"<< core->getCoreID()<<" { \n";
        m_file << "style=filled;\n";
	m_file<< "color=lightgrey;\n";
        m_file << "\tlabel = \"Core " << core->getCoreID() << " \";\n";
        for(const auto& model : core->m_indexed_models){
                m_file << '\t'<<model->getName() << "  ;\n";
        }
        // Close the cluster, connections can span clusters and listing below keeps the .dot slightly more readable.
        m_file << "\t}\n";
        for(const auto& model : core->m_indexed_models){
                std::string from = model->getName();
                for(const auto& oport : model->getOPorts()){
                        for(const auto& dest : oport->getCoupledOuts()){
                                AtomicModel_impl* model = dynamic_cast<AtomicModel_impl*>(dest.first->getHost());
                                std::string to = model->getName();
                                size_t otherid= model->getCoreID();
                                m_file << from << " -> " << to;
                                if(core->m_coreid != otherid)
                                        m_file<<" [ltail=cluster_"<<core->m_coreid<<",lhead=cluster_" << otherid << "]";
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

