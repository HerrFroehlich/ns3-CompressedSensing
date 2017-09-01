
CsClusterHeader::NcCoeff::NcCoeff(CsClusterHeader::NcCoeff::E_Type type, uint64_t stream) : m_type(type)
{
	if (type == E_Type::NORMAL)
	{
		Ptr<NormalRandomVariable> ranvar = CreateObject<NormalRandomVariable>();
		
	}
	else
		NS_ABORT_MSG("Invalid type");
}