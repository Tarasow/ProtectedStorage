#include "ProtectedStorage.h"
#include "VirtualFilesystem.h"
#include "GostHash.h"
#include <cstdlib>
#include <thread>
#include <future>


ProtectedStorage* ProtectedStorage::pInstance = nullptr;

ProtectedStorage* ProtectedStorage::getInstance() {
	if (pInstance == nullptr)
		pInstance = new ProtectedStorage();
	return pInstance;
}

ProtectedStorage::ProtectedStorage()
	: mMountdir(""), mIsMounted(false)
{
}

ProtectedStorage::~ProtectedStorage() {
	this->destroyStorage();
}

storageCreateStatus ProtectedStorage::createStorage(char** data, int size) { // тут еще strings вполне, std::map очень даже, если не хочется 5 параметров передавать
	if (mIsMounted == true)
		return storageCreateStatus::alreadyCreated;

	if (data == nullptr)
		return storageCreateStatus::noArguments;

	if (size < 4)
		return storageCreateStatus::fewArguments; // bad design

	std::string password = data[size - 1];

	auto key8 = std::make_unique<uint8_t[]>(32), // comma operator?!
			pass = std::make_unique<uint8_t[]>(password.length());
	for (size_t i = 0; i < password.length(); ++i)
		pass[i] = static_cast<uint8_t>(password[i]);

	// тут длина в битах? зачем?
	gosthash::hash256(pass.get(), 8*password.length(), key8.get());

	vfsState* vfss = new vfsState;
	for (int i = 0; i < 8; ++i) // всем этим манипуляциям место в отдельной функции password2key
		for (int j = 0; j < 4; ++j)
			vfss->key[i] += (static_cast<uint32_t>(key8[i*4 + j])) << (24 - 8*j);

	vfss->rootdir = realpath(data[size - 3], NULL); // never!!! data[size - 3]
	data[size - 3] = data[size - 2];
	size -= 2;

	std::packaged_task<int()>
			task([=] {return fuse_main_real(size, data, &vfsOper, sizeof(vfsOper), vfss); });
	auto fut = task.get_future();

	std::thread fuseThread(std::move(task));

	// тут что-то ненормальное, вдруг у меня очень быстрый компьютер?!
	auto status = fut.wait_for(std::chrono::milliseconds(100));

	fuseThread.detach();

	if (status == std::future_status::ready)
		return storageCreateStatus::errorInCreating;

	mMountdir = data[size]; // в нормальных домах это за пределами массива
	mIsMounted = true;
	return storageCreateStatus::successfullyCreated;
}

storageDestroyStatus ProtectedStorage::destroyStorage() {
	if (mIsMounted == false)
		return storageDestroyStatus::nothingToDestroy;
	std::string command = "fusermount -u ";
	command.append(mMountdir);
	if (system(command.c_str()) != 0) {
		return storageDestroyStatus::errorInDestroying;
	}
	mMountdir = "";
	mIsMounted = false;
	return storageDestroyStatus::successfullyDestroyed;
}

bool ProtectedStorage::isMounted() const {
	return mIsMounted;
}
