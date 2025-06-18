// Fill out your copyright notice in the Description page of Project Settings.


#include "Database/DatabaseManager.h"
#include "Subsystems/SubsystemBlueprintLibrary.h"


// Sets default values
ADatabaseManager::ADatabaseManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADatabaseManager::BeginPlay()
{
	Super::BeginPlay();

	// VaRestInit("https://www.damien-harle.com/UE5Getdata.html");
	FetchDataFromAPI("random_strings");
	FString test = GetDataFromJson("random_strings", "id", "string_col5", "10");
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Test: %s"), *test));
}

// Called every frame
void ADatabaseManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// void ADatabaseManager::VaRestTest(UVaRestRequestJSON* Request)
// {
// 	FString Response = Request->GetResponseContentAsString(false);
//
// 	FString Message = FString();
// 	
// 	Message.Append(TEXT("Request went through"));
// 	Message.Append(TEXT(" Response is: "));
// 	//the answer is a json so formatting it to be readable with line breaks
// 	Message.Append(Response.Replace(TEXT(","), TEXT(",\n")));
//
// 	GEngine->AddOnScreenDebugMessage(-1, 9.f, FColor::Yellow, Message);
// }
//
// void ADatabaseManager::VaRestInit(FString URL)
// {
// 	UVaRestSubsystem* Varestsub = GetVARestSub();
//
// 	if (!::IsValid(Varestsub))
// 	{
// 		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("VArest Subsystem missing"));
//
// 		return;
// 	}
//
// 	//This will bind VaRestTest function to VarestDelegate, which is how you get the result from CallUrl
// 	VarestDelegate.BindUFunction(this, FName("VaRestTest"));
//
// 	UVaRestJsonObject* tableToGet = NewObject<UVaRestJsonObject>();
// 	tableToGet->SetStringField("test", "test");
//        
// 	//This is the Call Url Function
// 	Varestsub->UVaRestSubsystem::CallURL(URL, EVaRestRequestVerb::GET, EVaRestRequestContentType::x_www_form_urlencoded_url, tableToGet, VarestDelegate);
//
// }
//
// UVaRestSubsystem* ADatabaseManager::GetVARestSub()
// {
// 	UVaRestSubsystem* Subsystem{};
//
//
// 	Subsystem = CastChecked<UVaRestSubsystem>(USubsystemBlueprintLibrary::GetEngineSubsystem(UVaRestSubsystem::StaticClass()), ECastCheckedType::NullAllowed);
//
// 	if (::IsValid(Subsystem))
// 	{
// 		return Subsystem;
// 	}
//
// 	return nullptr;
// }

void ADatabaseManager::FetchDataFromAPI(FString TableName)
{
    FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
    
    FString Url = FString::Printf(TEXT("https://damien-harle.com/api/ue5data?table=%s"), *TableName);
    
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));
    CurrentTableName = TableName;
    Request->OnProcessRequestComplete().BindUObject(this, &ADatabaseManager::OnResponseReceived);
    Request->ProcessRequest();
}

void ADatabaseManager::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to connect to API"));
        UE_LOG(LogTemp, Error, TEXT("Failed to connect to API"));
        return;
    }
    
    FString ResponseString = Response->GetContentAsString();
    
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
    
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        if (JsonObject->HasField(TEXT("success")) && JsonObject->GetBoolField(TEXT("success")))
        {
            // API returns data in a "data" array
            TArray<TSharedPtr<FJsonValue>> DataArray = JsonObject->GetArrayField(TEXT("data"));
            int32 RecordCount = DataArray.Num();
            
            // Log success
            // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
            //     FString::Printf(TEXT("Data fetched: %d records"), RecordCount));

        	//save Json in "Database Value" folder in the Saved directory if it exists if not create it
        	FString SaveDirectory = FPaths::ProjectSavedDir() / TEXT("Database Value");
        	if (!FPaths::DirectoryExists(SaveDirectory))
        	{
        		FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*SaveDirectory);
        		// Show message if directory was created
        		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
				// 	FString::Printf(TEXT("Directory created: %s"), *SaveDirectory));
        	}
        	FString FileName = FString::Printf(TEXT("%s.json"), *CurrentTableName);
        	FString FilePath = FPaths::Combine(SaveDirectory, FileName);
        	FFileHelper::SaveStringToFile(ResponseString, *FilePath);
        	// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
			// 	FString::Printf(TEXT("Data saved to: %s"), *FilePath));
        	
            
            // Process data
            for (int32 i = 0; i < DataArray.Num(); i++)
            {
                TSharedPtr<FJsonObject> Record = DataArray[i]->AsObject();
                
                if (Record->HasField(TEXT("string_col1")))
                {
                    FString StringValue = Record->GetStringField(TEXT("string_col1"));
                    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, 
                    //     FString::Printf(TEXT("Record %d: %s"), i, *StringValue));
                }
            }
        }
        else
        {
            // API returned an error
            FString ErrorMessage = JsonObject->HasField(TEXT("error")) ? 
                JsonObject->GetStringField(TEXT("error")) : TEXT("Unknown API error");
            
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, ErrorMessage);
            UE_LOG(LogTemp, Error, TEXT("API Error: %s"), *ErrorMessage);
        }
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to parse JSON response"));
        UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON: %s"), *ResponseString);
    }
}

FString ADatabaseManager::GetDataFromJson(FString JsonName, FString PrimaryKey, FString Column, FString PrimaryKeyValue)
{
	FString Result;
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("Database Value") / (JsonName + TEXT(".json"));
	
	if (!FPaths::FileExists(FilePath))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
			FString::Printf(TEXT("File not found: %s"), *FilePath));
		return Result;
	}

	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *FilePath))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, 
			FString::Printf(TEXT("Failed to load file: %s"), *FilePath));
		return Result;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);
	if (FJsonSerializer::Deserialize(Reader, JsonObject))
	{
		if (JsonObject->HasField(TEXT("data")))
		{
			TArray<TSharedPtr<FJsonValue>> DataArray = JsonObject->GetArrayField(TEXT("data"));
			for (const auto& Item : DataArray)
			{
				TSharedPtr<FJsonObject> Record = Item->AsObject();
				if (Record.IsValid())
				{
					if (Record->HasField(PrimaryKey) && Record->HasField(Column))
					{
						// FString PrimaryValue = Record->GetStringField(PrimaryKey);
						// FString SecondaryValue = Record->GetStringField(SecondaryKey);
						// Result.Add(FString::Printf(TEXT("%s: %s"), *PrimaryValue, *SecondaryValue));
						if (PrimaryKeyValue.IsEmpty() || Record->GetStringField(PrimaryKey) == PrimaryKeyValue)
						{
							Result = Record->GetStringField(Column);
							// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, 
							// 	FString::Printf(TEXT("Found: %s = %s"), *PrimaryKey, *Result));
							return Result;
						}
					}
				}
			}
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No 'data' field in JSON"));
			return Result;
		}
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Failed to parse JSON"));
	}

	return Result;
}
