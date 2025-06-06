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

	VaRestInit("http://10.51.51.202:8080/UE5Getdata.php");
	
}

// Called every frame
void ADatabaseManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ADatabaseManager::VaRestTest(UVaRestRequestJSON* Request)
{
	FString Response = Request->GetResponseContentAsString(false);

	FString Message = FString();
	
	Message.Append(TEXT("Request went through"));
	Message.Append(TEXT(" Response is: "));
	//the answer is a json so formatting it to be readable with line breaks
	Message.Append(Response.Replace(TEXT(","), TEXT(",\n")));

	GEngine->AddOnScreenDebugMessage(-1, 9.f, FColor::Yellow, Message);
}

void ADatabaseManager::VaRestInit(FString URL)
{
	UVaRestSubsystem* Varestsub = GetVARestSub();

	if (!::IsValid(Varestsub))
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.f, FColor::Red, TEXT("VArest Subsystem missing"));

		return;
	}

	//This will bind VaRestTest function to VarestDelegate, which is how you get the result from CallUrl
	VarestDelegate.BindUFunction(this, FName("VaRestTest"));

	UVaRestJsonObject* tableToGet = NewObject<UVaRestJsonObject>();
	tableToGet->SetStringField("table", "test");
       
	//This is the Call Url Function
	Varestsub->UVaRestSubsystem::CallURL(URL, EVaRestRequestVerb::GET, EVaRestRequestContentType::x_www_form_urlencoded_url, tableToGet, VarestDelegate);

}

UVaRestSubsystem* ADatabaseManager::GetVARestSub()
{
	UVaRestSubsystem* Subsystem{};


	Subsystem = CastChecked<UVaRestSubsystem>(USubsystemBlueprintLibrary::GetEngineSubsystem(UVaRestSubsystem::StaticClass()), ECastCheckedType::NullAllowed);

	if (::IsValid(Subsystem))
	{
		return Subsystem;
	}

	return nullptr;
}


