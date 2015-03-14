# John input file format

## XML Schema

<?xml version="1.0" encoding="utf-8"?>
<xs:schema>

	<xs:element name="AFL">
		<xs:annotation>
			<xs:documentation>Root element of a John XML DDL document.
			</xs:documentation>
		</xs:annotation>
		<xs:complexType>
			<xs:choice minOccurs="1" maxOccurs="unbounded">
				   <xs:element ref="DataElement" />
			</xs:choice>
		</xs:complexType>
	</xs:element>

	<xs:element name="DataElement">
		<xs:annotation>
			<xs:documentation>It is a container and it contains other elements including DataElement.
			</xs:documentation>
                </xs:annotation>
	</xs:element>

	<xs:element name="string">
		<xs:annotation>
			<xs:documentation>A string of characters.
	  		</xs:documentation>
		</xs:annotation>
		<xs:complexType>
			<xs:attribute name="type">
				   <xs:annotation>
						<xs:documentation>Specify type of string. Default is STR.
						</xs:documentation>
				   </xs:annotation>
				   <xs:simpleType>
						<xs:restriction base="xs:string">
								<xs:enumeration value="STR">
										<xs:annotation>
											<xs:documentation>String of characters
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="NUM">
										<xs:annotation>
											<xs:documentation>String of characters which is from '0' to '9'
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="HEX_STR">
										<xs:annotation>
											<xs:documentation>String of hex characters, contains botn upper case and lower case
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="HEX_STR_U">
										<xs:annotation>
											<xs:documentation>String of hex characters, contains only upper case
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="HEX_STR_L">
										<xs:annotation>
											<xs:documentation>String of hex characters, contains only lower case
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
						</xs:restriction>
				   </xs:simpleType>
		        </xs:attribute>
			<xs:attribute name="length">
				   <xs:annotation>
						<xs:documentation>The length of the characters. length="40": the length must be 40; length="[min,max]": the length is from min to max; length="[,max]" the length is from 0 to max; length="[min,]": the length is from min to STRING_MAX_SIZE
						</xs:documentation>
				   </xs:annotation>
			</xs:attribute>
			<xs:attribute name="is_mutate">
				   <xs:annotation>
						<xs:documentation>Is current element should be mutated
						</xs:documentation>
				   </xs:annotation>
				   <xs:simpleType>
						<xs:restriction base="xs:string">
								<xs:enumeration value="true">
										<xs:annotation>
											<xs:documentation>Default value, this element can be mutated
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
								<xs:enumeration value="false">
										<xs:annotation>
											<xs:documentation>Never mutate this element
											</xs:documentation>
										</xs:annotation>
								</xs:enumeration>
						</xs:restriction>
				   </xs:simpleType>
			</xs:attribute>
	</xs:element>	

</xs:schema>


## Strategy





## Built-in functions




## Samples


### siemens-s7.xml





