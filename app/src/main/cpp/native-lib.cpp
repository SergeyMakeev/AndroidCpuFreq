#include <jni.h>
#include <string>
#include <iostream>
#include <sstream>

#include "../../../../CpuFreq/CpuFreq.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_cpufreqtest_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */)
{
    // fetch info
    std::vector<CpuFreq::CpuInfo> cpus;
    cpus.reserve(16);
    CpuFreq::readCpuInfo(cpus);

    // print it
    std::ostringstream out;

    for(size_t i = 0; i < cpus.size(); i++) {
        const CpuFreq::CpuInfo& info = cpus[i];
        out << "CPU: " << info.cpuIndex << " Freq\n";
        out << "Min : " << info.minFreq << ", ";
        out << "Max : " << info.maxFreq << ", ";
        out << "Current : " << info.currentFreq << "\n";
        out << "------------------------------\n";
    }

    return env->NewStringUTF(out.str().c_str());
}