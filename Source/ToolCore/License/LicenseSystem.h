//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

// BEGIN LICENSE MANAGEMENT

#pragma once

#include <Atomic/Core/Object.h>
#include "../Net/CurlManager.h"

using namespace Atomic;

namespace ToolCore
{

class LicenseSystem : public Object
{
    OBJECT(LicenseSystem);

public:

    struct LicenseParse
    {
        bool licenseWindows_;
        bool licenseMac_;
        bool licenseAndroid_;
        bool licenseIOS_;
        bool licenseHTML5_;
        bool licenseModule3D_;

        LicenseParse()
        {
            licenseWindows_ = false;
            licenseMac_ = false;
            licenseAndroid_ = false;
            licenseIOS_ = false;
            licenseHTML5_ = false;
            licenseModule3D_ = false;
        }
    };


    /// Construct.
    LicenseSystem(Context* context);
    /// Destruct.
    virtual ~LicenseSystem();

    void Initialize();

    bool GetSourceBuild();

    bool GetLicenseWindows() { return licenseWindows_; }
    bool GetLicenseMac() { return licenseMac_; }
    bool GetLicenseAndroid() { return licenseAndroid_; }
    bool GetLicenseIOS() { return licenseIOS_; }
    bool GetLicenseHTML5() { return licenseHTML5_; }
    bool GetLicenseModule3D() { return licenseModule3D_; }

    /// Returns whether there are any platform licenses available
    bool IsStandardLicense();

    void Activate(const String& key, const LicenseParse& parse);

    /// Returns true if request to deactivate is made
    bool Deactivate();

    void ResetLicense();
    bool LoadLicense();

    /// Basic key validation
    bool ValidateKey(const String &key);

    /// Activate on server
    void RequestServerActivation(const String& key);

    const String& GetKey() { return key_; }
    String GenerateMachineID();
    String GetEmail() { return email_;}

    void LicenseAgreementConfirmed();

    int ParseResponse(const String& response, LicenseParse &parse);

private:

    void RequestServerVerification(const String& key);

    void SaveLicense();
    void RemoveLicense();

    void CreateOrUpdateLicenseCache();

    void HandleVerification(StringHash eventType, VariantMap& eventData);
    void HandleDeactivate(StringHash eventType, VariantMap& eventData);
    void HandleEditorShutdown(StringHash eventType, VariantMap& eventData);

    void HandleActivationResult(StringHash eventType, VariantMap& eventData);

    bool eulaAgreementConfirmed_;

    String licenseFilePath_;
    String licenseCachePath_;
    String eulaAgreementPath_;

    String key_;
    String email_;
    bool licenseWindows_;
    bool licenseMac_;
    bool licenseAndroid_;
    bool licenseIOS_;
    bool licenseHTML5_;
    bool licenseModule3D_;

    SharedPtr<CurlRequest> serverActivation_;
    SharedPtr<CurlRequest> serverVerification_;
    SharedPtr<CurlRequest> deactivate_;


};

}

// END LICENSE MANAGEMENT
