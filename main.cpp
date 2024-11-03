#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <fstream>
#include <queue>

namespace fs = std::filesystem;
const int TOP_WORDS_COUNT = 100;

// Function to count words and filter them based on the rules
std::unordered_map<std::string, int> countWords(const std::string& filename, int& totalWords) {
    std::unordered_map<std::string, int> wordCount;
    std::ifstream file(filename);
    std::string word;
    std::unordered_set<std::string> commonWords = {"A", "AND", "AN", "OF", "IN", "THE"};

    while (file >> word) {
        // Remove non-alphanumeric characters and convert to uppercase
        word.erase(std::remove_if(word.begin(), word.end(), [](char c) { return !isalnum(c); }), word.end());
        std::transform(word.begin(), word.end(), word.begin(), ::toupper);

    }
    return wordCount;
}

// Function to normalize and select the top words
std::unordered_map<std::string, double> getTopNormalizedWords(std::unordered_map<std::string, int>& wordCount, int totalWords) {
    std::vector<std::pair<std::string, int>> wordVector(wordCount.begin(), wordCount.end());

    // Sort by frequency in descending order
    std::sort(wordVector.begin(), wordVector.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    // Take the top 100 words and normalize
    std::unordered_map<std::string, double> topWords;
    for (int i = 0; i < std::min(TOP_WORDS_COUNT, (int)wordVector.size()); i++) {
        const auto& [word, count] = wordVector[i];
        topWords[word] = static_cast<double>(count) / totalWords;
    }

    return topWords;
}

// Process all books and get their word profiles
std::unordered_map<std::string, std::unordered_map<std::string, double>> processBooks(const std::vector<std::string>& filenames) {
    std::unordered_map<std::string, std::unordered_map<std::string, double>> bookProfiles;

    for (const auto& filename : filenames) {
        int totalWords = 0;
        auto wordCount = countWords(filename, totalWords);
        bookProfiles[filename] = getTopNormalizedWords(wordCount, totalWords);
    }

    return bookProfiles;
}

// Create similarity matrix
std::vector<std::vector<double>> createSimilarityMatrix(const std::vector<std::string>& filenames,
    const std::unordered_map<std::string, std::unordered_map<std::string, double>>& bookProfiles) {

    int bookCount = filenames.size();
    std::vector<std::vector<double>> similarityMatrix(bookCount, std::vector<double>(bookCount, 0.0));

    for (int i = 0; i < bookCount; ++i) {
        for (int j = i + 1; j < bookCount; ++j) {
            const auto& profileA = bookProfiles.at(filenames[i]);
            const auto& profileB = bookProfiles.at(filenames[j]);
            double similarityIndex = 0.0;

            // Calculate similarity index for common words
            for (const auto& [word, freqA] : profileA) {
                if (profileB.count(word)) {
                    similarityIndex += freqA * profileB.at(word);
                }
            }

            similarityMatrix[i][j] = similarityIndex;
            similarityMatrix[j][i] = similarityIndex; // Reflect across the diagonal
        }
    }

    return similarityMatrix;
}

// Find top 10 most similar pairs from the similarity matrix
std::vector<std::tuple<int, int, double>> findTopSimilarPairs(const std::vector<std::vector<double>>& similarityMatrix, int topPairsCount = 10) {
    int bookCount = similarityMatrix.size();
    std::priority_queue<std::tuple<double, int, int>> maxHeap;

    // Use a max heap to store pairs based on similarity
    for (int i = 0; i < bookCount; ++i) {
        for (int j = i + 1; j < bookCount; ++j) {
            maxHeap.emplace(similarityMatrix[i][j], i, j);
        }
    }

    // Retrieve the top pairs
    std::vector<std::tuple<int, int, double>> topPairs;
    for (int i = 0; i < topPairsCount && !maxHeap.empty(); ++i) {
        auto [similarity, bookA, bookB] = maxHeap.top();
        maxHeap.pop();
        topPairs.emplace_back(bookA, bookB, similarity);
    }

    return topPairs;
}

int main() {
    // Load filenames into vector
    std::string folderPath = "Book-Txt";
    std::vector<std::string> filenames;
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        filenames.push_back(entry.path().string());
    }

    // Process books and get word profiles
    auto bookProfiles = processBooks(filenames);

    // Create similarity matrix
    std::vector<std::vector<double>> similarityMatrix = createSimilarityMatrix(filenames, bookProfiles);

    // Find and display the top 10 most similar pairs
    auto topPairs = findTopSimilarPairs(similarityMatrix);
    for (const auto& [bookA, bookB, similarity] : topPairs) {
        std::cout << "Similarity between \"" << filenames[bookA] << "\" and \"" << filenames[bookB]
                  << "\" is " << similarity << '\n';
    }

    return 0;
}
