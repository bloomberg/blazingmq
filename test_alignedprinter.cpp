// Simple test for our AlignedPrinter refactor
#include <bmqu_alignedprinter.h>
#include <bsl_vector.h>
#include <bsl_string.h>
#include <bsl_iostream.h>
#include <bsl_sstream.h>

int main() {
    // Test with the new bsl::string interface
    bsl::vector<bsl::string> fields;
    fields.push_back("Queue URI");
    fields.push_back("QueueKey");
    fields.push_back("Number of AppIds");

    bsl::stringstream output;
    const int indent = 4;
    bmqu::AlignedPrinter printer(output, &fields, indent);

    // Test printing values
    bsl::string uri = "bmq://bmq.tutorial.workqueue/sample-queue";
    bsl::string queueKey = "sample";
    const int num = 1;
    
    printer << uri << queueKey << num;

    // Print result
    bsl::cout << "AlignedPrinter Test Output:\n" << output.str() << bsl::endl;
    
    return 0;
}