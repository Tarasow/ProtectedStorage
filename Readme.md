# PROTECTED STORAGE

## About

Protected storage 0.92 RC

Created on C++ using FUSE library and Qt

It provides the functionality of creating and using protected storage with any data.
The mounted directory is similar to an regular logical disk.


## Configure

### Remember: this program can run only on unix systems. You also need Qt framework (https://www.qt.io) and FUSE library (https://github.com/libfuse/libfuse/releases/tag/fuse-2.9.7) installed on your system. 

If you have Qt Creator IDE, simply open as project ProtectedStorage.pro and run.


## How to use

At first, fill in all the fields:

* Root directory - where you want to store\nyour encrypted data.

* Mount directory - where you want to work with it.

* Password - need to encrypt and decrypt.

Then, push the button "Create storage" to create it.

To destroy the storage, push the button "Destroy storage".

#### Remember: you can't create more than one storage at the same time.

#### If an error occurs, the program will report about it and offer some solutions.


## I wish you will find the using of this program convenient and useful.

### 2016, Tarasov Kirill, https://github.com/Tarasow/ProtectedStorage
