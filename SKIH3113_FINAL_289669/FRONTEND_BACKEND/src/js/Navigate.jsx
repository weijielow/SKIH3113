import React from 'react';
import Container from 'react-bootstrap/Container';
import Navbar from 'react-bootstrap/Navbar';
import { Link } from 'react-router-dom';

function Navigate() {
  return (
    <>
      <Navbar className="bg-body-tertiary">
        <Container>
          <Navbar.Brand as={Link} to="/read" className="center-text">Tacking</Navbar.Brand>
        </Container>
      </Navbar>
      <Navbar className="bg-body-tertiary">
        <Container>
          <Navbar.Brand as={Link} to="/display" className="center-text">Analyse</Navbar.Brand>
        </Container>
      </Navbar>
    </>
  );
}

export default Navigate;
