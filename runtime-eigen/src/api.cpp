#include "../include/differential_privacy_runtime_eigen/api.hpp"
#include "../include/differential_privacy_runtime_eigen/base.hpp"

extern "C" char* release(
        char* analysisBuffer, size_t analysisLength,
        char* releaseBuffer, size_t releaseLength,
        char* dataPath, size_t dataPathLength,
        char* header, size_t headerLength) {

    // parse analysis from protocol buffer
    std::string analysisString(analysisBuffer, analysisLength);
    Analysis analysisProto;
    analysisProto.ParseFromString(analysisString);

    std::string releaseString(releaseBuffer, releaseLength);
    Release releaseProto;
    releaseProto.ParseFromString(releaseString);

    std::string dataPathString(dataPath, dataPathLength);

    // construct eigen matrix from double pointers
    auto matrix = load_csv(dataPathString);

    // get column names from char pointer
    auto* columns = new std::vector<std::string>();
    std::string headerStr(header, headerLength);

    size_t position = 0;
    std::string delimiter(",");

    while ((position = headerStr.find(delimiter)) != std::string::npos) {
        columns->push_back(headerStr.substr(0, position));
        headerStr.erase(0, position + delimiter.length());
    }

    // DEBUGGING
    std::cout << "Analysis:\n" << analysisProto.DebugString();
    std::cout << "Release:\n" << releaseProto.DebugString();
    for (const auto & column : *columns) std::cout << column << ' ';
    std::cout << std::endl <<  matrix;


    // EXECUTION
    Release* releaseProtoAfter = executeGraph(analysisProto, releaseProto, matrix, *columns);

    std::cout << "Release After:\n" << releaseProtoAfter->DebugString();

    std::string releaseMessage = releaseProtoAfter->SerializeAsString();

    google::protobuf::ShutdownProtobufLibrary();
    return const_cast<char *>(releaseMessage.c_str());

//    strncpy(responseBuffer, responseBufferRaw, responseLength);
//    return releaseMessage.length();
}

extern "C" char* releaseArray(
        char* analysisBuffer, size_t analysisLength,
        char* releaseBuffer, size_t releaseLength,
        int m, int n, const double** data,
        char* header, size_t headerLength) {

    // parse analysis from protocol buffer
    std::string analysisString(analysisBuffer, analysisLength);
    Analysis analysisProto;
    analysisProto.ParseFromString(analysisString);

    std::string releaseString(releaseBuffer, releaseLength);
    Release releaseProto;
    releaseProto.ParseFromString(releaseString);

    // construct eigen matrix from double pointers
    Eigen::MatrixXd matrix(m, n);
    for (unsigned int i = 0; i < m; ++i)
        for (unsigned int j = 0; j < n; ++j)
            matrix(i, j) = data[i][j];

    // get column names from char pointer
    auto* columns = new std::vector<std::string>();
    std::string headerStr(header, headerLength);

    size_t position = 0;
    std::string delimiter(",");

    while ((position = headerStr.find(delimiter)) != std::string::npos) {
        columns->push_back(headerStr.substr(0, position));
        headerStr.erase(0, position + delimiter.length());
    }

    // DEBUGGING
    std::cout << "Analysis:\n" << analysisProto.DebugString();
    std::cout << "Release:\n" << releaseProto.DebugString();
    for (const auto & column : *columns) std::cout << column << ' ';
    std::cout << std::endl <<  matrix;

    // EXECUTION
    Release* releaseProtoAfter = executeGraph(analysisProto, releaseProto, matrix, *columns);

    std::cout << "Release After:\n" << releaseProtoAfter->DebugString();

    std::string releaseMessage = releaseProtoAfter->SerializeAsString();

    google::protobuf::ShutdownProtobufLibrary();
    return const_cast<char *>(releaseMessage.c_str());

//    strncpy(responseBuffer, responseBufferRaw, responseLength);
//    return releaseMessage.length();
}