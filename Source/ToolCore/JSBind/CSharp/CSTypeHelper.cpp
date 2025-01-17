

#include "../JSBPackage.h"
#include "CSTypeHelper.h"

namespace ToolCore
{

void CSTypeHelper::GenNativeFunctionParameterSignature(JSBFunction* function, String& sig)
{
    JSBClass* klass = function->GetClass();

    Vector<JSBFunctionType*>& parameters = function->GetParameters();

    Vector<String> args;

    if (!function->IsConstructor())
    {
        args.Push(ToString("%s* self", klass->GetNativeName().CString()));
    }

    if (parameters.Size())
    {
        for (unsigned int i = 0; i < parameters.Size(); i++)
        {
            JSBFunctionType* ptype = parameters.At(i);

            // ignore "Context" parameters
            if (ptype->type_->asClassType())
            {
                JSBClassType* classType = ptype->type_->asClassType();
                JSBClass* klass = classType->class_;
                if (klass->GetName() == "Context")
                {
                    continue;
                }

                args.Push(ToString("%s* %s", klass->GetNativeName().CString(), ptype->name_.CString()));
            }
            else
            {
                args.Push(CSTypeHelper::GetNativeTypeString(ptype) + " " + ptype->name_);
            }

        }
    }

    if (function->GetReturnClass() && function->GetReturnClass()->IsNumberArray())
    {
        args.Push(ToString("%s* returnValue", function->GetReturnClass()->GetNativeName().CString()));
    }

    sig.Join(args, ", ");

}

String CSTypeHelper::GetNativeFunctionSignature(JSBFunction* function, String& returnType)
{

    if (function->Skip())
        return String::EMPTY;

    if (function->IsDestructor())
        return String::EMPTY;

    if (OmitFunction(function))
        return String::EMPTY;

    JSBClass* klass = function->GetClass();
    JSBPackage* package = klass->GetPackage();
    String fname = function->IsConstructor() ? "Constructor" : function->GetName();

    returnType = "void";

    if (function->IsConstructor())
    {
        returnType = "RefCounted*";
    }
    else if (function->GetReturnType())
    {
        if (function->IsConstructor())
        {
            returnType = ToString("%s*", klass->GetNativeName().CString());
        }
        else if (function->GetReturnClass())
        {
            if (!function->GetReturnClass()->IsNumberArray())
            {
                returnType = ToString("const %s*", function->GetReturnClass()->GetNativeName().CString());
            }
        }
        else if (function->GetReturnType()->type_->asStringHashType())
        {
            returnType = "unsigned";
        }
        else
        {
            returnType = ToString("%s", CSTypeHelper::GetNativeTypeString(function->GetReturnType()).CString());
        }
    }


    String sig;
    GenNativeFunctionParameterSignature(function, sig);

    String functionSig = ToString("csb_%s_%s_%s(%s)",
                package->GetName().CString(), klass->GetName().CString(),
                fname.CString(), sig.CString());

    return functionSig;
}

String CSTypeHelper::GetManagedPrimitiveType(JSBPrimitiveType* ptype)
{
    if (ptype->kind_ == JSBPrimitiveType::Bool)
        return "bool";
    if (ptype->kind_ == JSBPrimitiveType::Int && ptype->isUnsigned_)
        return "uint";
    else if (ptype->kind_ == JSBPrimitiveType::Int)
        return "int";
    if (ptype->kind_ == JSBPrimitiveType::Float)
        return "float";
    if (ptype->kind_ == JSBPrimitiveType::Char && ptype->isUnsigned_)
        return "byte";
    else if (ptype->kind_ == JSBPrimitiveType::Char)
        return "char";
    if (ptype->kind_ == JSBPrimitiveType::Short)
        return "short";

    assert(0);
    return "";
}


String CSTypeHelper::GetManagedTypeString(JSBType* type)
{
    String value;

    if (type->asClassType())
    {
        JSBClassType* classType = type->asClassType();
        value = classType->class_->GetName();
    }
    else if (type->asStringType() || type->asStringHashType())
    {
        value = "string";
    }
    else if (type->asEnumType())
    {
        value = type->asEnumType()->enum_->GetName();
    }
    else if (type->asPrimitiveType())
    {
        value = GetManagedPrimitiveType(type->asPrimitiveType());
    }
    else if (type->asVectorType())
    {
        JSBVectorType* vectorType = type->asVectorType();

        value = GetManagedTypeString(vectorType->vectorType_) + "[]";
    }

    return value;
}

String CSTypeHelper::GetManagedTypeString(JSBFunctionType* ftype, bool addName)
{
    if (!ftype)
        return "void";

    String parameter = GetManagedTypeString(ftype->type_);

    if (ftype->name_.Length())
    {
        if (addName)
        {
            if (ftype->name_ == "object")
            {
                parameter += " _object";
            }
            else if (ftype->name_ == "readonly")
            {
                parameter += " readOnly";
            }
            else if (ftype->name_ == "params")
            {
                parameter += " parameters";
            }
            else
            {
                parameter += " " + ftype->name_;
            }

        }

        /*
        if (ftype->initializer_.Length())
            parameter += " = " + ftype->initializer_;
        */
    }

    return parameter;

}

String CSTypeHelper::GetNativeTypeString(JSBType* type)
{
    String value;

    if (type->asClassType())
    {
        JSBClassType* classType = type->asClassType();
        if (classType->class_->IsNumberArray())
            value = ToString("%s*", classType->class_->GetNativeName().CString());
    }
    else if (type->asStringType())
    {
        value = "const char*";
    }
    else if (type->asStringHashType())
    {
        value = "const char*";
    }
    else if (type->asEnumType())
    {
        value = type->asEnumType()->enum_->GetName();
    }
    else if (type->asPrimitiveType())
    {
        value = type->asPrimitiveType()->ToString();
    }
    else if (type->asVectorType())
    {
        assert(0);
    }

    return value;
}

String CSTypeHelper::GetNativeTypeString(JSBFunctionType* ftype)
{
    if (!ftype)
        return "void";

    String value = GetNativeTypeString(ftype->type_);

    return value;

}

String CSTypeHelper::GetPInvokeTypeString(JSBType* type)
{
    String value;

    if (type->asClassType())
    {
        JSBClassType* classType = type->asClassType();
        if (classType->class_->IsNumberArray())
            value = "void";
        else
            value = "IntPtr";
    }
    else if (type->asStringType() || type->asStringHashType())
    {
        value = "string";
    }
    else if (type->asEnumType())
    {
        value = type->asEnumType()->enum_->GetName();
    }
    else if (type->asPrimitiveType())
    {
        value = GetManagedPrimitiveType(type->asPrimitiveType());
    }
    else if (type->asVectorType())
    {
        JSBVectorType* vectorType = type->asVectorType();

        value = GetManagedTypeString(vectorType->vectorType_) + "[]";
    }

    return value;
}

String CSTypeHelper::GetPInvokeTypeString(JSBFunctionType* ftype)
{
    if (!ftype)
        return "void";

    String value = GetPInvokeTypeString(ftype->type_);

    return value;

}

bool CSTypeHelper::IsSimpleReturn(JSBType* type)
{
    if (type->asClassType())
    {
        return false;
    }
    else if (type->asStringType() || type->asStringHashType())
    {
        return true;
    }
    else if (type->asEnumType())
    {
        return true;
    }
    else if (type->asPrimitiveType())
    {
        return true;
    }
    else if (type->asVectorType())
    {
        return true;
    }

    return true;
}

bool CSTypeHelper::IsSimpleReturn(JSBFunctionType* ftype)
{
    if (!ftype)
        return true;

    return IsSimpleReturn(ftype->type_);

}

bool CSTypeHelper::OmitFunction(JSBFunction* function)
{
    if (!function)
        return false;

    if (function->Skip())
        return true;

    if (function->IsDestructor())
        return true;

    // We need to rename GetType
    if (function->GetName() == "GetType")
        return true;

    // avoid vector type for now
    if (function->GetReturnType() && function->GetReturnType()->type_->asVectorType())
        return true;

    Vector<JSBFunctionType*>& parameters = function->GetParameters();

    for (unsigned i = 0; i < parameters.Size(); i++)
    {
        if (parameters[i]->type_->asVectorType())
            return true;
    }

    return false;
}


}
