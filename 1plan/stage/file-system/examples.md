# File System Utilities - Usage Examples

## Basic Operations

### File Existence Checking
```cpp
#include "util/infra/file/path.h"

// Check if a file exists
if (infra_file::isExistFile("data/config.txt")) {
    // File exists, proceed with reading
    LOG("Configuration file found");
} else {
    LOG_ERR("Configuration file not found");
}
```

### Directory Creation
```cpp
#include "util/infra/file/path.h"

// Create directory (including parents if needed)
std::filesystem::path data_dir = "project/data";
if (infra_file::ensureFolder(data_dir)) {
    LOG("Directory created successfully");
} else {
    LOG_ERR("Failed to create directory");
}
```

### File Copying
```cpp
#include "util/infra/file/path.h"

// Copy file with overwrite
std::filesystem::path source = "source.txt";
std::filesystem::path destination = "backup.txt";

if (infra_file::copyTo(source, destination)) {
    LOG("File copied successfully");
} else {
    LOG_ERR("File copy failed");
}
```

## Directory Operations

### Recursive File Listing
```cpp
#include "util/infra/file/path.h"
#include <iostream>

// List all files in directory recursively
std::vector<std::filesystem::path> all_files;
if (infra_file::getFileInDir("project", all_files)) {
    LOG("Found " << all_files.size() << " files");
    for (const auto& file : all_files) {
        LOG_VAR(file.string());
    }
} else {
    LOG_ERR("Failed to list files");
}
```

### Extension-Based File Filtering
```cpp
#include "util/infra/file/path.h"

// List all .txt files in directory
std::vector<std::filesystem::path> txt_files;
if (infra_file::getFileInDir("documents", ".txt", txt_files)) {
    LOG("Found " << txt_files.size() << " text files");
    for (const auto& file : txt_files) {
        LOG_VAR(file.string());
    }
} else {
    LOG_ERR("Failed to list text files");
}
```

### Directory Listing
```cpp
#include "util/infra/file/path.h"

// List all subdirectories
std::vector<std::filesystem::path> subdirs;
if (infra_file::getDirInDir("project", subdirs)) {
    LOG("Found " << subdirs.size() << " subdirectories");
    for (const auto& dir : subdirs) {
        LOG_VAR(dir.string());
    }
} else {
    LOG_ERR("Failed to list directories");
}
```

## Advanced Usage

### File Cleanup
```cpp
#include "util/infra/file/path.h"
#include <vector>

// Clean up temporary files
std::vector<std::filesystem::path> temp_files;
if (infra_file::getFileInDir("temp", ".tmp", temp_files)) {
    for (const auto& file : temp_files) {
        if (infra_file::deleteFile(file)) {
            LOG("Deleted temporary file: " << file.string());
        } else {
            LOG_ERR("Failed to delete: " << file.string());
        }
    }
}
```

### Directory Structure Validation
```cpp
#include "util/infra/file/path.h"

// Ensure required directory structure exists
std::vector<std::filesystem::path> required_dirs = {
    "project/src",
    "project/include", 
    "project/build",
    "project/tests"
};

bool structure_ok = true;
for (const auto& dir : required_dirs) {
    if (!infra_file::ensureFolder(dir)) {
        LOG_ERR("Failed to create directory: " << dir);
        structure_ok = false;
    }
}

if (structure_ok) {
    LOG("Project structure created successfully");
}
```

### File Organization
```cpp
#include "util/infra/file/path.h"
#include <regex>

// Organize files by extension
std::filesystem::path source_dir = "downloads";
std::map<std::string, std::filesystem::path> extension_dirs = {
    {".jpg", "images"},
    {".png", "images"},
    {".mp3", "audio"},
    {".mp4", "video"}
};

std::vector<std::filesystem::path> files;
if (infra_file::getFileInDir(source_dir, files)) {
    for (const auto& file : files) {
        std::string ext = file.extension().string();
        if (extension_dirs.count(ext)) {
            std::filesystem::path target_dir = extension_dirs[ext];
            infra_file::ensureFolder(target_dir);
            infra_file::copyTo(file, target_dir / file.filename());
        }
    }
}
```

## Error Handling Examples

### Robust File Operations
```cpp
#include "util/infra/file/path.h"
#include <iostream>

void safeFileOperation(const std::filesystem::path& file_path) {
    // Check if file exists first
    if (!infra_file::isExistFile(file_path)) {
        LOG_ERR("File does not exist: " << file_path.string());
        return;
    }

    // Try to copy file
    std::filesystem::path backup_path = file_path.string() + ".bak";
    if (!infra_file::copyTo(file_path, backup_path)) {
        LOG_ERR("Failed to create backup: " << backup_path.string());
        return;
    }

    LOG("Backup created successfully");
}
```

### Directory Creation with Error Handling
```cpp
#include "util/infra/file/path.h"

bool createDirectoryStructure(const std::vector<std::filesystem::path>& dirs) {
    for (const auto& dir : dirs) {
        if (!infra_file::ensureFolder(dir)) {
            LOG_ERR("Failed to create directory: " << dir.string());
            return false;
        }
    }
    return true;
}
```

## Performance Optimization Examples

### Batch File Operations
```cpp
#include "util/infra/file/path.h"
#include <vector>

void batchFileProcessing(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    if (!infra_file::getFileInDir(dir, files)) {
        LOG_ERR("Failed to list files");
        return;
    }

    // Process files in batches
    const size_t batch_size = 100;
    for (size_t i = 0; i < files.size(); i += batch_size) {
        size_t end = std::min(i + batch_size, files.size());
        
        // Process batch of files
        for (size_t j = i; j < end; ++j) {
            // Process file files[j]
            LOG_VAR(files[j].string());
        }
        
        LOG("Processed batch " << (i/batch_size + 1));
    }
}
```

### Memory-Efficient Directory Traversal
```cpp
#include "util/infra/file/path.h"

void processLargeDirectory(const std::filesystem::path& dir) {
    std::vector<std::filesystem::path> files;
    if (!infra_file::getFileInDir(dir, files)) {
        LOG_ERR("Failed to list files");
        return;
    }

    // Process files one by one to minimize memory usage
    for (const auto& file : files) {
        // Process individual file
        LOG_VAR(file.string());
        
        // Clear memory if needed for very large directories
        if (files.size() > 10000) {
            files.clear();
            // Continue processing with different strategy
        }
    }
}
```

## Integration Examples

### With Debug Utilities
```cpp
#include "util/infra/file/path.h"
#include "util/infra/debug/log.hpp"

void debugFileSystemOperations() {
    TIME_BLOCK_BEGIN(file_operations);
    
    std::filesystem::path test_dir = "test_directory";
    
    LOG("Creating directory: " << test_dir.string());
    if (infra_file::ensureFolder(test_dir)) {
        LOG("Directory created successfully");
    } else {
        LOG_ERR("Directory creation failed");
    }
    
    // Create test file
    std::filesystem::path test_file = test_dir / "test.txt";
    std::ofstream file(test_file);
    file << "Test content";
    file.close();
    
    LOG("Created test file: " << test_file.string());
    
    // Verify file exists
    if (infra_file::isExistFile(test_file)) {
        LOG("File verification successful");
    } else {
        LOG_ERR("File verification failed");
    }
    
    TIME_BLOCK_END(file_operations);
}
```

### With Testing Framework
```cpp
#include "util/infra/file/path.h"
#include "doctest/doctest.h"

TEST_CASE("File System Utilities") {
    std::filesystem::path test_dir = "test_dir";
    
    // Setup
    REQUIRE(infra_file::ensureFolder(test_dir));
    
    // Test file creation
    std::filesystem::path test_file = test_dir / "test.txt";
    std::ofstream file(test_file);
    file << "test";
    file.close();
    
    // Test file existence
    REQUIRE(infra_file::isExistFile(test_file));
    
    // Test file copy
    std::filesystem::path copy_file = test_dir / "copy.txt";
    REQUIRE(infra_file::copyTo(test_file, copy_file));
    REQUIRE(infra_file::isExistFile(copy_file));
    
    // Cleanup
    REQUIRE(infra_file::deleteFile(test_file));
    REQUIRE(infra_file::deleteFile(copy_file));
    REQUIRE(infra_file::deleteFolder(test_dir));
}
```

## Best Practices

### 1. Always Check Return Values
```cpp
// Good
if (infra_file::ensureFolder(dir)) {
    // Proceed with operation
} else {
    // Handle error
}

// Bad - ignoring return values
infra_file::ensureFolder(dir); // What if this fails?
```

### 2. Use Proper Error Handling
```cpp
// Good - comprehensive error handling
if (!infra_file::copyTo(source, dest)) {
    LOG_ERR("Copy failed: " << source.string() << " -> " << dest.string());
    return false;
}

// Bad - silent failure
infra_file::copyTo(source, dest); // User won't know if it failed
```

### 3. Use Path Objects, Not Strings
```cpp
// Good - use std::filesystem::path
std::filesystem::path file_path = "data/config.txt";
if (infra_file::isExistFile(file_path)) {
    // ...
}

// Bad - use raw strings
if (infra_file::isExistFile("data/config.txt")) {
    // Hard to maintain and modify
}
```

### 4. Clean Up Resources
```cpp
// Good - ensure cleanup
void processFile(const std::filesystem::path& file) {
    if (!infra_file::isExistFile(file)) {
        LOG_ERR("File not found: " << file.string());
        return;
    }
    
    // Process file...
    // Cleanup happens automatically when function exits
}
```

### 5. Use Appropriate Logging
```cpp
// Good - informative logging
LOG("Processing file: " << file_path.string());
if (infra_file::copyTo(file_path, backup_path)) {
    LOG("Backup created: " << backup_path.string());
} else {
    LOG_ERR("Backup failed: " << file_path.string());
}

// Bad - insufficient logging
infra_file::copyTo(file_path, backup_path); // No feedback
```