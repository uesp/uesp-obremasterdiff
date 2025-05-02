#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>
#include "tes4lib.h"

#pragma comment(lib, "legacy_stdio_definitions.lib")


void DumpFormids(std::vector<obformid_t>& formids, CObSimpleRecordHandler& espFile)
{
	std::unordered_map<std::string, std::vector<obformid_t>> formIdGroups;
	CSString Buffer;
	CSString Buffer1;
	CSString Buffer2;

	for (auto id : formids)
	{
		auto pRecord = espFile.FindFormID(id);
		if (pRecord) formIdGroups[std::string(pRecord->GetHeader().RecordType.Name, 4)].push_back(id);
	}

	for (auto i : formIdGroups)
	{
		auto recType = i.first;
		auto& formids = i.second;

		SystemLog.Printf("\t\t%4.4s Records (%d)", recType.c_str(), formids.size());

		for (auto id : formids)
		{
			auto pRecord = espFile.FindFormID(id);

			pRecord->GetField(Buffer, OB_FIELD_EDITORID);
			pRecord->GetField(Buffer1, OB_FIELD_FULLNAME);
			if (Buffer1.IsEmpty()) pRecord->GetField(Buffer1, OB_FIELD_ITEMNAME);

			if (pRecord->GetHeader().RecordType == OB_NAME_REFR)
			{
				pRecord->GetField(Buffer2, OB_FIELD_BASEEDITORID);
				if (Buffer.IsEmpty()) Buffer = Buffer1;

				if (Buffer.IsEmpty()) 
					SystemLog.Printf("\t\t\t0x%08X (%4.4s) = %s", id, pRecord->GetHeader().RecordType.Name, Buffer2);
				else
					SystemLog.Printf("\t\t\t0x%08X (%4.4s) = %s (%s%s%s)", id, pRecord->GetHeader().RecordType.Name, Buffer2, Buffer, Buffer1.IsEmpty() ? "" : " / ", Buffer1);
			}
			else if (pRecord->GetHeader().RecordType == OB_NAME_CELL)
			{
				int pos;
				auto pXclc = pRecord->FindFirstSubrecord("XCLC", pos);

				if (pXclc)
					Buffer2.Format("(%d, %d)", ((dword *)pXclc->GetData())[0], ((dword *)pXclc->GetData())[1]);
				else
					Buffer2 = Buffer1;

				if (Buffer.IsEmpty())
					SystemLog.Printf("\t\t\t0x%08X (%4.4s) = %s %s", id, pRecord->GetHeader().RecordType.Name, "Exterior", Buffer2);
				else
					SystemLog.Printf("\t\t\t0x%08X (%4.4s) = %s %s", id, pRecord->GetHeader().RecordType.Name, Buffer, Buffer2);
			}
			else if (Buffer1.IsEmpty())
			{
				SystemLog.Printf("\t\t\t0x%08X (%4.4s) = %s", id, pRecord->GetHeader().RecordType.Name, Buffer);
			}
			else
			{
				SystemLog.Printf("\t\t\t0x%08X (%4.4s) = %s / %s", id, pRecord->GetHeader().RecordType.Name, Buffer, Buffer1);
			}
		}
	}

}


void CompareFiles(CObSimpleRecordHandler& file1, CObSimpleRecordHandler& file2)
{
	OBMAPPOS pos;
	CObRecord* pRecord;
	struct _stat64 fileStat1;
	struct _stat64 fileStat2;
	int counter = 0;
	int counter2 = 0;
	int recordCounter = 0;
	int groupCounter = 0;
	int recordCounter2 = 0;
	int groupCounter2 = 0;
	int missingRecords = 0;
	int newRecords = 0;
	CSString Buffer;
	CSString Buffer1;
	std::vector<obformid_t> missingFormids;
	std::vector<obformid_t> newFormids;

	_stat64(file1.GetEspFile().GetFilename(), &fileStat1);
	_stat64(file2.GetEspFile().GetFilename(), &fileStat2);
	SystemLog.Printf("File Names: %s / %s", file1.GetEspFile().GetFilename(), file2.GetEspFile().GetFilename());
	SystemLog.Printf("File Size: %llu / %llu (%lld) bytes", fileStat1.st_size, fileStat2.st_size, fileStat2.st_size - fileStat1.st_size);
	printf("File Names: %s / %s\n", file1.GetEspFile().GetFilename(), file2.GetEspFile().GetFilename());
	printf("File Size: %llu / %llu (%lld) bytes\n", fileStat1.st_size, fileStat2.st_size, fileStat2.st_size - fileStat1.st_size);

	SystemLog.Printf("Num Records: %d / %d (%d)", file1.GetNumRecords(), file2.GetNumRecords(), file2.GetNumRecords() - file1.GetNumRecords());
	printf("Num Records: %d / %d (%d)\n", file1.GetNumRecords(), file2.GetNumRecords(), file2.GetNumRecords() - file1.GetNumRecords());

	SystemLog.Printf("Iterating records from file #1...");
	printf("Iterating records from file #1...\n");
	pRecord = file1.GetFirstRecord(pos);

	while (pRecord)
	{
		++counter;

		if (!pRecord->IsGroup())
		{
			auto formId = pRecord->GetFormID();
			auto pRecord2 = file2.FindFormID(formId);

			if (pRecord2 == nullptr)
			{
				++missingRecords;
				missingFormids.push_back(formId);
			}

			++recordCounter;
		}
		else
		{
			++groupCounter;
		}

		pRecord = file1.GetNextRecord(pos);
	}

	SystemLog.Printf("\tFound %d total records in file #1 (%d records, %d groups)!", counter, recordCounter, groupCounter);
	SystemLog.Printf("\tMissing Records in File2 = %d", missingRecords);
	SystemLog.Printf("\t==============================================================");
	printf("\tFound %d total records in file #1 (%d records, %d groups)!\n", counter, recordCounter, groupCounter);
	printf("\tMissing Records in File2 = %d\n", missingRecords);

	DumpFormids(missingFormids, file1);

	SystemLog.Printf("Iterating records from file #2...");
	printf("Iterating records from file #2...\n");
	pRecord = file2.GetFirstRecord(pos);

	while (pRecord)
	{
		++counter2;

		if (!pRecord->IsGroup())
		{
			auto formId = pRecord->GetFormID();
			auto pRecord1 = file1.FindFormID(formId);

			if (pRecord1 == nullptr)
			{
				++newRecords;
				newFormids.push_back(formId);
			}

			++recordCounter2;
		}
		else
		{
			++groupCounter2;
		}

		pRecord = file2.GetNextRecord(pos);
	}

	SystemLog.Printf("\tFound %d total records in file #2 (%d records, %d groups)!", counter2, recordCounter2, groupCounter2);
	SystemLog.Printf("\tNew Records in File2 = %d", newRecords);
	SystemLog.Printf("\t==============================================================");
	printf("\tFound %d total records in file #2 (%d records, %d groups)!\n", counter2, recordCounter2, groupCounter2);
	printf("\tNew Records in File2 = %d\n", newRecords);

	DumpFormids(newFormids, file2);

	SystemLog.Printf("\tComparing Record Data...");
	SystemLog.Printf("\t==============================================================");
	printf("\tComparing Record Data...");
	pRecord = file2.GetFirstRecord(pos);

	while (pRecord)
	{
		if (!pRecord->IsGroup())
		{
			auto formId = pRecord->GetFormID();
			auto pRecord1 = file1.FindFormID(formId);

			if (pRecord1 != nullptr)
			{
				pRecord->GetUserData();
			}
		}

		pRecord = file2.GetNextRecord(pos);
	}
}


int main()
{
	CObSimpleRecordHandler TestFile;
	CObSimpleRecordHandler TestFile1;

	SystemLog.Open("ObRemaster.log");

	printf("Loading file 1...\n");
	auto start_time = std::chrono::high_resolution_clock::now();

	TestFile.Load("D:\\Oblivion\\Data\\Oblivion.esm");

	auto end_time = std::chrono::high_resolution_clock::now();
	auto deltatime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	printf("\tFinished in %llu ms\n", deltatime.count());

	printf("Loading file 2...\n");
	start_time = std::chrono::high_resolution_clock::now();

	TestFile1.Load("D:\\Oblivion\\Data\\OblivionRemaster.esm");

	end_time = std::chrono::high_resolution_clock::now();
	deltatime = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
	printf("\tFinished in %llu ms\n", deltatime.count());

	CompareFiles(TestFile, TestFile1);

	return 0;
}

