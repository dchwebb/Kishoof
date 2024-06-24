#pragma once

class GpioPin {
public:
	enum class Type {Input, InputPullup, InputPulldown, Output, AlternateFunction};
	enum class DriveStrength {Low, Medium, High, VeryHigh};

	GpioPin(GPIO_TypeDef* port, uint32_t pin, Type pinType, uint32_t alternateFunction = 0, DriveStrength driveStrength = DriveStrength::Low) :
		port(port), pin(pin), pinType(pinType)
	{
		Init(port, pin, pinType, alternateFunction, driveStrength);		// Init function is static so can be called without instantiating object
	}


	static void Init(GPIO_TypeDef* port, const uint32_t pin, const Type pinType, const uint32_t alternateFunction = 0, const DriveStrength driveStrength = DriveStrength::Low)
	{
		// maths to calculate RCC clock to enable
		const uint32_t portPos = ((uint32_t)port - SRD_AHB4PERIPH_BASE) >> 10;
		RCC->AHB4ENR |= (1 << portPos);

		// 00: Input, 01: Output, 10: Alternate function, 11: Analog (reset state)
		if (pinType == Type::Input || pinType == Type::InputPullup || pinType == Type::InputPulldown) {
			port->MODER &= ~(0b11 << (pin * 2));
			if (pinType == Type::InputPullup) {
				port->PUPDR |= (0b1 << (pin * 2));
			}
			if (pinType == Type::InputPulldown) {
				port->PUPDR |= (0b10 << (pin * 2));
			}

		} else if (pinType == Type::Output) {
			port->MODER |=  (0b01 << (pin * 2));
			port->MODER &= ~(0b10 << (pin * 2));

		} if (pinType == Type::AlternateFunction) {
			port->MODER |=  (0b10 << (pin * 2));
			port->MODER &= ~(0b01 << (pin * 2));
			if (pin < 8) {
				port->AFR[0] |= alternateFunction << (pin * 4);
			} else {
				port->AFR[1] |= alternateFunction << ((pin - 8) * 4);
			}
		}

		port->OSPEEDR |= static_cast<uint32_t>(driveStrength) << (pin * 2);
	}


	static bool IsHigh(GPIO_TypeDef* port, const uint32_t pin) {
		return (port->IDR & (1 << pin));
	}

	bool IsHigh() {
		return (port->IDR & (1 << pin));
	}

	static bool IsLow(GPIO_TypeDef* port, const uint32_t pin) {
		return ((port->IDR & (1 << pin)) == 0);
	}

	bool IsLow() {
		return ((port->IDR & (1 << pin)) == 0);
	}

	static void SetHigh(GPIO_TypeDef* port, const uint32_t pin) {
		port->ODR |= (1 << pin);
	}

	void SetHigh() {
		port->ODR |= (1 << pin);
	}

	static void SetLow(GPIO_TypeDef* port, const uint32_t pin) {
		port->ODR &= ~(1 << pin);
	}

	void SetLow() {
		port->ODR &= ~(1 << pin);
	}

private:
	GPIO_TypeDef* port;
	uint32_t pin;
	Type pinType;
};

