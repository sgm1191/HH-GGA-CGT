#include "stagnation_dump_observer.hpp"

#include <stdexcept>
#include <utility>

#include "paths.hpp"

namespace ggacgt {

StagnationDumpObserver::StagnationDumpObserver(int configId, std::string instanceBaseName)
    : configId_(configId), instanceBaseName_(std::move(instanceBaseName)) {}

void StagnationDumpObserver::onGeneration(const PopulationStats& /*stats*/, const ParameterSet& /*params*/,
                                           const GenerationOperatorStats& /*operatorStats*/) {}

void StagnationDumpObserver::onStagnationDetected(const Population& population, GenerationNumber generation) {
    if (!out_.is_open()) {
        out_.open(paths::stagnationDumpFile(configId_, instanceBaseName_), std::ios::app);
        if (!out_) throw std::runtime_error("Could not open stagnation dump file for " + instanceBaseName_);
    }

    long bestIdx = population.orderedIndex(population.size() - 1);
    const Chromosome& best = population.at(bestIdx);

    out_ << "===== Muta Generacion " << generation << " | Mejor solucion (indice " << bestIdx << ") =====\n";
    out_ << "Total de bins utilizados: " << best.binsUsed << ", Aptitud: " << best.fitness
         << ", Generacion: " << best.birthGeneration << "\n";

    for (std::size_t i = 0; i < best.bins.size(); ++i) {
        const Bin& bin = best.bins[i];
        out_ << "Bin " << i << " (uso: " << bin.fullness() << "): ";
        if (bin.items().empty()) {
            out_ << "(vacio)\n";
        } else {
            for (ItemId item : bin.items()) out_ << item << " ";
            out_ << "\n";
        }
    }

    out_ << "\n-- Estructura general de la poblacion (aptitud y bins llenos) --\n";
    for (long p = 0; p < population.size(); ++p) {
        const Chromosome& individual = population.at(p);
        out_ << "Ind " << p << " | Aptitud: " << individual.fitness
             << " | Bins llenos al 100%: " << individual.fullBinCount << "\n";
    }
    out_ << "\n";
    out_.flush();
}

} // namespace ggacgt
