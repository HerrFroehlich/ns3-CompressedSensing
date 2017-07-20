/**
* \file template-registration.h
*
* \author Tobias Waurick
* \date 19.07.17
*
* defines a makro to register templates in ns3, such a makro is defined in only in the current dev version
*/
#include <string>
#include <complex>

#ifndef TEMPLATE_REGISTRATION_H
#define TEMPLATE_REGISTRATION_H

#define OBJECT_TEMPLATE_CLASS_DEFINE(type, param)        \
	template class type<param>;                          \
	static struct Object##type##param##RegistrationClass \
	{                                                    \
		Object##type##param##RegistrationClass()         \
		{                                                \
			ns3::TypeId tid = type<param>::GetTypeId();  \
			tid.SetSize(sizeof(type<param>));            \
			tid.GetParent();                             \
		}                                                \
	} Object##type##param##RegistrationVariable

/*template class type<param>; /*/
template <typename T>
inline std::string GetTypeParamName(void);

template <>
inline std::string GetTypeParamName<float>()
{
	return "float";
}

template <>
inline std::string GetTypeParamName<double>()
{
	return "double";
}

template <>
inline std::string GetTypeParamName<std::complex<double>>()
{
	return "cx_double";
}

template <>
inline std::string GetTypeParamName<std::complex<uint8_t>>()
{
	return "uint8";
}

template <>
inline std::string GetTypeParamName<std::complex<float>>()
{
	return "cx_float";
}
template <>
inline std::string GetTypeParamName<std::complex<uint16_t>>()
{
	return "uint16";
}

template <>
inline std::string GetTypeParamName<std::complex<uint32_t>>()
{
	return "uint32";
}

#endif //TEMPLATE_REGISTRATION_H