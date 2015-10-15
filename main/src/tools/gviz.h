#ifndef SRC_TOOLS_GVIZ_H_
#define SRC_TOOLS_GVIZ_H_

#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

// Fwd decl iso full include.
namespace n_model{
class Core;
}

namespace n_tools{

/**
 * A thread safe visualization tool for the simulator.
 */
class GVizWriter{
private:
        std::mutex m_mu;
        std::ofstream m_file;
        const std::string m_config_node;
        
        //@throws ios_base::failure
        GVizWriter(const std::string& outname);
        GVizWriter()=delete;
        GVizWriter(const GVizWriter&)=delete;
        GVizWriter(const GVizWriter&&)=delete;
        
        void
        writeHeader();
        void
        writeFooter();
public:
        /**
         * Get the writer object. If it doesn't exist (yet), create it.
         * @param fname : only makes sense if the object does not exist, other calls will not 
         * use the parameter, the first caller sets the file.
         * @throws bad_alloc, ios_base::failure
         */
        static
        GVizWriter* getWriter(const std::string fname);
        
        ~GVizWriter();
        
        void
        writeObject(n_model::Core* core);

};

}

#endif