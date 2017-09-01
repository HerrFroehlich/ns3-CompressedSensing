
#ifndef NC_COEFF_GENERATOR
#define NC_COEFF_GENERATOR

class NcCoeffGenerator
{
	class NcCoeff
	{
	  public:
		enum E_Type /**< enum stating which type of values is used for network coding*/
		{
			NORMAL, /**< using normal distributed coefficients*/
			BERN	/**< using bernoulli distributed(-1/+1) coefficients*/
		};

		/**
		* \brief create a network coefficient with the given type and a given stream index number
		*
		* \param type type of nc coefficients
		* \param ranvar random variable stream
		*/
		NcCoeff(E_Type type, Ptr<RandomVariable> ranvvar);

	  private:
		E_Type m_type;
		double m_valNorm; /**< value when using normal*/
		int8_t m_valBern; /**< value when using bernoulli*/
	};
};

#endif //NC_COEFF_GENERATOR
